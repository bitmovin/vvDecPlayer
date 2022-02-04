/* MIT License

Copyright (c) 2021 Christian Feldmann <christian.feldmann@gmx.de>
                                      <christian.feldmann@bitmovin.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "ViewWidget.h"

#include <QException>
#include <QOpenGLShader>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QTimerEvent>
#include <assert.h>

#define DEBUG_WIDGET 0
#if DEBUG_WIDGET
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

constexpr auto    INFO_MESSAGE_TIMEOUT        = std::chrono::seconds(10);
static const auto SEMGENT_LENGTH_FRAMES_GUESS = 24u;
constexpr auto    ATTRIB_VERTEX               = 0;
constexpr auto    ATTRIB_TEXTURE              = 1;

class OpenGlException : public QException
{
public:
  void             raise() const { throw *this; }
  OpenGlException *clone() const { return new OpenGlException(*this); }
};

} // namespace

ViewWidget::ViewWidget(QWidget *parent) : QOpenGLWidget(parent)
{
  auto pal = this->palette();
  pal.setColor(QPalette::Window, Qt::black);
  this->setAutoFillBackground(true);
  this->setPalette(pal);
}

void ViewWidget::setPlaybackController(PlaybackController *playbackController)
{
  assert(playbackController != nullptr);
  this->playbackController = playbackController;
}

void ViewWidget::addMessage(QString message, LoggingPriority priority)
{
  std::scoped_lock lock(this->messagesMutex);

  ViewWidgetMessage msg;
  msg.message   = message;
  msg.priority  = priority;
  msg.timeAdded = std::chrono::steady_clock::now();
  this->messages.push_back(msg);
}

// void ViewWidget::paintEvent(QPaintEvent *)
// {
//   QPainter painter(this);
//   painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);

//   DEBUG("Paint event");

//   this->drawCurrentFrame(painter);
//   this->drawAndUpdateMessages(painter);
//   this->drawFPSAndStatusText(painter);
//   this->drawRenditionInfo(painter);
//   this->drawProgressGraph(painter);
// }

void ViewWidget::initializeGL()
{
  initializeOpenGLFunctions();
  glEnable(GL_DEPTH_TEST);

  this->vertexShader = std::make_unique<QOpenGLShader>(QOpenGLShader::Vertex, this);

  const char *vertexShaderSource = "attribute vec4 vertexIn; \
        attribute vec2 textureIn; \
        varying vec2 textureOut;  \
        void main(void)           \
        {                         \
            gl_Position = vertexIn; \
            textureOut = textureIn; \
        }";

  if (!this->vertexShader->compileSourceCode(vertexShaderSource))
    throw OpenGlException();

  this->fragmentShader             = std::make_unique<QOpenGLShader>(QOpenGLShader::Fragment, this);
  const char *fragmentShaderSource = "varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
  if (!this->fragmentShader->compileSourceCode(fragmentShaderSource))
    throw OpenGlException();

  this->shaderProgram = std::make_unique<QOpenGLShaderProgram>(this);
  // TODO: do addShaderFromSourceCode
  this->shaderProgram->addShader(this->fragmentShader.get());
  this->shaderProgram->addShader(this->vertexShader.get());
  this->shaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
  this->shaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

  if (!this->shaderProgram->link() || !this->shaderProgram->bind())
    throw OpenGlException();

  this->textureUniformY = this->shaderProgram->uniformLocation("tex_y");
  this->textureUniformU = this->shaderProgram->uniformLocation("tex_u");
  this->textureUniformV = this->shaderProgram->uniformLocation("tex_v");

  static const GLfloat vertexVertices[] = {
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      1.0f,
  };

  static const GLfloat textureVertices[] = {
      0.0f,
      1.0f,
      1.0f,
      1.0f,
      0.0f,
      0.0f,
      1.0f,
      0.0f,
  };

  glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
  glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
  glEnableVertexAttribArray(ATTRIB_VERTEX);
  glEnableVertexAttribArray(ATTRIB_TEXTURE);

  // Create y, u, v texture objects respectively
  this->textureY = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
  this->textureU = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
  this->textureV = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
  this->textureY->create();
  this->textureU->create();
  this->textureV->create();

  this->textureIdY = this->textureY->textureId();
  this->textureIdU = this->textureU->textureId();
  this->textureIdV = this->textureV->textureId();

  glClearColor(0.3, 0.3, 0.3, 0.0); // set the background color
}

void ViewWidget::resizeGL(int w, int h) {}

void ViewWidget::paintGL() {}

void ViewWidget::drawCurrentFrame(QPainter &painter)
{
  if (this->curFrame.isNull())
    return;

  auto &rgbImage = this->curFrame.frame->rgbImage;
  if (rgbImage.isNull())
    return;

  if (this->scaleVideo)
  {
    QRect drawRect;
    auto  aspectRatioImage  = double(rgbImage.width()) / double(rgbImage.height());
    auto  aspectRatioWidget = double(this->width()) / double(this->height());
    if (aspectRatioWidget > aspectRatioImage)
    {
      // Full height, black bars left and right
      drawRect.setHeight(this->height());
      drawRect.setWidth(aspectRatioImage * this->height());
      drawRect.moveTopLeft(QPoint((this->width() - drawRect.width()) / 2, 0));
    }
    else
    {
      // Full width, black bars top and bottom
      drawRect.setHeight(this->width() / aspectRatioImage);
      drawRect.setWidth(this->width());
      drawRect.moveTopLeft(QPoint(0, (this->height() - drawRect.height()) / 2));
    }
    painter.drawImage(drawRect, rgbImage);
  }
  else
  {
    int x = (this->width() - rgbImage.width()) / 2;
    int y = (this->height() - rgbImage.height()) / 2;
    painter.drawImage(x, y, rgbImage);
  }
}

void ViewWidget::drawAndUpdateMessages(QPainter &painter)
{
  unsigned           yOffset = 0;
  constexpr unsigned MARGIN  = 2;

  std::scoped_lock lock(this->messagesMutex);

  auto it = this->messages.begin();
  while (it != this->messages.end())
  {
    auto age = std::chrono::steady_clock::now() - it->timeAdded;
    if (it->priority == LoggingPriority::Info && age > INFO_MESSAGE_TIMEOUT)
    {
      it = this->messages.erase(it);
      continue;
    }

    auto text     = it->message;
    auto textSize = QFontMetrics(painter.font()).size(0, text);

    QRect textRect;
    textRect.setSize(textSize);
    textRect.moveTopLeft(QPoint(MARGIN, yOffset + MARGIN));

    // Draw the colored background box
    auto       boxRect = textRect + QMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    const auto colorMap =
        std::map<LoggingPriority, QColor>({{LoggingPriority::Info, Qt::cyan},
                                           {LoggingPriority::Warning, Qt::darkYellow},
                                           {LoggingPriority::Error, Qt::darkRed}});
    painter.setBrush(colorMap.at(it->priority));
    painter.drawRect(boxRect);

    painter.drawText(textRect, Qt::AlignRight, text);

    yOffset += boxRect.height();
    it++;
  }
}

void ViewWidget::drawFPSAndStatusText(QPainter &painter)
{
  auto text = QString("FPS: %1\n").arg(this->actualFPS);
  if (this->playbackController && this->showDebugInfo)
    text += this->playbackController->getStatus();
  auto textSize = QFontMetrics(painter.font()).size(0, text);

  if (this->showDebugInfo)
  {
    if (textSize.width() > this->debugInfoRenderMaxWidth)
      this->debugInfoRenderMaxWidth = textSize.width();
    else
      textSize.setWidth(this->debugInfoRenderMaxWidth);
  }

  QRect textRect;
  textRect.setSize(textSize);
  textRect.moveTopRight(QPoint(this->width(), 0));

  painter.setPen(Qt::white);
  painter.drawText(textRect, Qt::AlignLeft, text);
}

void ViewWidget::drawRenditionInfo(QPainter &painter)
{
  auto manifest = this->playbackController->getManifest();
  if (manifest == nullptr || this->curFrame.isNull())
    return;

  auto renditions               = manifest->getRenditionInfos();
  auto currentTargetRendition   = manifest->getCurrentRencodition();
  auto currentPlaybackRendition = this->curFrame.segment->segmentInfo.rendition;

  const auto arrawText     = "-->";
  const auto arrowTextSize = QFontMetrics(painter.font()).size(0, arrawText);
  QRect      arrowTextRect;
  arrowTextRect.setSize(arrowTextSize);
  arrowTextRect.setLeft(0);

  unsigned topPos = 0;
  for (auto i = int(renditions.size() - 1); i >= 0; i--)
  {
    const auto &rendition = renditions.at(i);

    if (i == currentPlaybackRendition)
      painter.setPen(Qt::green);
    else
      painter.setPen(Qt::white);

    if (i == currentTargetRendition)
    {
      arrowTextRect.moveTop(topPos);
      painter.drawText(arrowTextRect, Qt::AlignLeft, arrawText);
    }

    auto text = QString("%1 - %2x%3@%4")
                    .arg(rendition.name)
                    .arg(rendition.resolution.width)
                    .arg(rendition.resolution.height)
                    .arg(rendition.fps);

    auto textSize = QFontMetrics(painter.font()).size(0, text);

    QRect textRect;
    textRect.setSize(textSize);
    textRect.moveLeft(arrowTextRect.width());
    textRect.moveTop(topPos);

    painter.drawText(textRect, Qt::AlignLeft, text);

    topPos += textRect.height();
  }
}

void ViewWidget::drawProgressGraph(QPainter &painter)
{
  if (!this->playbackController || !this->showProgressGraph)
    return;

  QRectF graphRect(0, 0, 500, 300);
  graphRect.moveBottomLeft(QPoint(0, this->height()));

  // painter.setBrush(Qt::white);
  // painter.drawRect(graphRect);

  const auto colorMap = std::map<FrameState, QColor>({{FrameState::Empty, Qt::lightGray},
                                                      {FrameState::Decoded, Qt::blue},
                                                      {FrameState::ConvertedToRGB, Qt::green}});

  auto bufferState =
      this->playbackController->getSegmentBuffer()->getBufferStatusForRender(this->curFrame.frame);
  if (bufferState.empty())
    return;

  const auto spaceBetweenFrames   = 2.0;
  const auto spaceBetweenSegments = 1.0;
  const auto segmentBoxBoarder    = QSizeF(0.5, 1.0);

  auto frameRect = QRectF(QPointF(0, 0), QSizeF(5, 5));
  frameRect.moveBottom(graphRect.bottom() - 5);

  auto firstSegmentLeft = 5;
  if (auto offset = bufferState.at(0).indexOfCurFrameInFrames)
    firstSegmentLeft -= *offset * (frameRect.width() + spaceBetweenFrames);

  {
    QRectF segmentRect;
    segmentRect.setHeight(frameRect.height() + segmentBoxBoarder.height() * 2);
    segmentRect.moveBottom(frameRect.bottom() + segmentBoxBoarder.height());

    auto segmentLeft = firstSegmentLeft;

    auto firstSegmentNrFrames = SEMGENT_LENGTH_FRAMES_GUESS;
    if (bufferState.size() > 0 && bufferState[0].nrFrames > 0)
      firstSegmentNrFrames = bufferState[0].nrFrames;

    painter.setPen(Qt::NoPen);
    for (auto &segment : bufferState)
    {
      auto nrFrames = segment.nrFrames;
      if (nrFrames == 0)
        nrFrames = firstSegmentNrFrames;

      segmentRect.setWidth(nrFrames * frameRect.width() + (nrFrames - 1) * spaceBetweenFrames +
                           segmentBoxBoarder.width() * 2);
      segmentRect.moveLeft(segmentLeft);

      painter.setBrush(Qt::white);
      painter.drawRect(segmentRect);

      // Draw the box for the segment
      {
        const auto maxBoxHeight = 100;
        auto       absHeight    = segment.sizeInBytes * 8 * maxBoxHeight / this->plotMaxBitrate;

        QRectF bitrateBarRect;

        bitrateBarRect.setWidth(nrFrames * frameRect.width() + (nrFrames - 1) * spaceBetweenFrames +
                                segmentBoxBoarder.width() * 2);
        bitrateBarRect.moveLeft(segmentLeft);
        bitrateBarRect.setHeight(absHeight * segment.downloadProgress / 100);
        bitrateBarRect.moveBottom(frameRect.top() - 5);
        painter.setBrush(Qt::darkCyan);
        painter.setPen(Qt::NoPen);
        painter.drawRect(bitrateBarRect);

        bitrateBarRect.setHeight(absHeight);
        bitrateBarRect.moveBottom(frameRect.top() - 5);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::cyan);
        painter.drawRect(bitrateBarRect);

        painter.setPen(Qt::black);
        painter.drawText(bitrateBarRect,
                         QString("R%1S%2").arg(segment.renditionNumber).arg(segment.segmentNumber),
                         Qt::AlignHCenter | Qt::AlignBottom);

        segmentLeft += bitrateBarRect.width() + spaceBetweenSegments;
      }

      // Draw the per frame boxes and bitrate
      auto frameLeft = segmentRect.left() + segmentBoxBoarder.width();
      for (auto &frameInfo : segment.frameInfo)
      {
        frameRect.moveLeft(frameLeft);
        painter.setBrush(colorMap.at(frameInfo.frameState));
        painter.drawRect(frameRect);

        auto frameBitrateRect = frameRect;
        auto absHeight        = frameInfo.sizeInBytes / 1000;
        frameBitrateRect.setHeight(absHeight * segment.downloadProgress / 100);
        frameBitrateRect.moveBottom(frameRect.top() - 5);
        painter.drawRect(frameBitrateRect);

        frameLeft += frameRect.width() + spaceBetweenFrames;
      }
    }
  }
}

void ViewWidget::clearMessages()
{
  std::scoped_lock lock(this->messagesMutex);
  this->messages.clear();
  this->update();
}

void ViewWidget::setScaleVideo(bool scaleVideo)
{
  this->scaleVideo = scaleVideo;
  this->update();
}

void ViewWidget::setShowDebugInfo(bool showDebugInfo)
{
  this->showDebugInfo = showDebugInfo;
  this->update();
}

void ViewWidget::setShowProgressGraph(bool showGraph)
{
  this->showProgressGraph = showGraph;
  this->update();
}

void ViewWidget::setPlaybackFps(double framerate)
{
  this->targetFPS = framerate;

  if (framerate == 0.0)
    timer.stop();
  else
  {
    auto timerInterval = int(1000.0 / this->targetFPS);
    timer.start(timerInterval, Qt::PreciseTimer, this);
  }
}

void ViewWidget::setPlotMaxBitrate(unsigned plotMaxBitrate)
{
  if (plotMaxBitrate > 0)
    this->plotMaxBitrate = plotMaxBitrate;
}

void ViewWidget::onPlayPause()
{
  if (this->targetFPS <= 0)
    return;

  this->pause = !this->pause;
}

void ViewWidget::onStep()
{
  if (this->pause)
    this->getNextFrame();
}

void ViewWidget::timerEvent(QTimerEvent *event)
{
  if (event && event->timerId() != timer.timerId())
    return QWidget::timerEvent(event);
  if (this->playbackController == nullptr)
    return;

  DEBUG("Timer event");

  if (!this->pause)
    this->getNextFrame();

  this->update();
}

void ViewWidget::getNextFrame()
{
  auto                         segmentBuffer = this->playbackController->getSegmentBuffer();
  SegmentBuffer::FrameIterator displayFrame;
  if (this->curFrame.isNull())
    displayFrame = segmentBuffer->getFirstFrameToDisplay();
  else
    displayFrame = segmentBuffer->getNextFrameToDisplay(this->curFrame);
  if (displayFrame.isNull())
  {
    DEBUG("Show next frame. No new image available.");
    return;
  }

  DEBUG("Show next frame. Got next image");
  this->curFrame = displayFrame;

  // Update the FPS counter every 50 frames
  this->timerFPSCounter++;
  if (this->timerFPSCounter > 50)
  {
    auto   newFrameTime         = QTime::currentTime();
    double msecsSinceLastUpdate = (double)this->timerLastFPSTime.msecsTo(newFrameTime);

    // Print the frames per second as float with one digit after the decimal dot.
    if (msecsSinceLastUpdate == 0)
      this->actualFPS = 0.0;
    else
      this->actualFPS = (50.0 / (msecsSinceLastUpdate / 1000.0));

    this->timerLastFPSTime = QTime::currentTime();
    this->timerFPSCounter  = 0;
  }

  this->frameSegmentOffset++;
  if (this->frameSegmentOffset > displayFrame.segment->nrFrames)
    this->frameSegmentOffset = 0;
}
