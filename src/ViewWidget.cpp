/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "ViewWidget.h"

#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QTimerEvent>

#define DEBUG_WIDGET 0
#if DEBUG_WIDGET
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

constexpr auto INFO_MESSAGE_TIMEOUT = std::chrono::seconds(10);

}

ViewWidget::ViewWidget(QWidget *parent) : QWidget(parent)
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

void ViewWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);

  if (this->curFrame.isNull())
    return;

  DEBUG("Paint event");

  auto &rgbImage = this->curFrame.frame->rgbImage;
  if (!rgbImage.isNull())
  {
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

  this->drawAndUpdateMessages(painter);
  this->drawFPSAndStatusText(painter);
  this->drawProgressGraph(painter);
}

void ViewWidget::drawAndUpdateMessages(QPainter &painter)
{
  unsigned           yOffset = 0;
  constexpr unsigned MARGIN  = 2;

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

  auto boxRect = textRect + QMargins(5, 5, 5, 5);
  painter.setPen(Qt::white);
  painter.drawText(textRect, Qt::AlignLeft, text);
}

void ViewWidget::drawProgressGraph(QPainter &painter)
{
  if (!this->playbackController || !this->showProgressGraph)
    return;

  QRectF graphRect(0, 0, 500, 300);
  graphRect.moveBottomLeft(QPoint(0, this->height()));

  // painter.setBrush(Qt::white);
  // painter.drawRect(graphRect);

  const auto colorMap = std::map<FrameState, QColor>(
      {{FrameState::Decoded, Qt::blue}, {FrameState::ConvertedToRGB, Qt::green}});

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

    painter.setPen(Qt::NoPen);
    for (auto &segment : bufferState)
    {
      segmentRect.setWidth(segment.nrFrames * frameRect.width() +
                           (segment.nrFrames - 1) * spaceBetweenFrames +
                           segmentBoxBoarder.width() * 2);
      segmentRect.moveLeft(segmentLeft);

      painter.setBrush(Qt::white);
      painter.drawRect(segmentRect);

      auto frameLeft = segmentRect.left() + segmentBoxBoarder.width();
      for (auto &frameState : segment.frameStates)
      {
        frameRect.moveLeft(frameLeft);
        painter.setBrush(colorMap.at(frameState));
        painter.drawRect(frameRect);
        frameLeft += frameRect.width() + spaceBetweenFrames;
      }

      segmentLeft += segmentRect.width() + spaceBetweenSegments;
    }
  }

  {
    // Next the bitrate graph
    auto segmentLeft = firstSegmentLeft;

    for (auto &segment : bufferState)
    {
      auto absHeight = segment.sizeInBytes / 1000;

      QRectF bitrateBarRect;
      bitrateBarRect.setWidth(segment.nrFrames * frameRect.width() +
                              (segment.nrFrames - 1) * spaceBetweenFrames +
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

      segmentLeft += bitrateBarRect.width() + spaceBetweenSegments;
    }
  }
}

void ViewWidget::clearMessages()
{
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
  if (this->frameSegmentOffset > 24)
    this->frameSegmentOffset = 0;
}
