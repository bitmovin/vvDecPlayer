/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "ViewWidget.h"

#include <QPainter>
#include <QPalette>
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

  double frameRate     = 24.0;
  auto   timerInterval = int(1000.0 / frameRate);
  timer.start(timerInterval, Qt::PreciseTimer, this);
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

  if (!this->currentImage.isNull())
  {
    int x = (this->width() - this->currentImage.width()) / 2;
    int y = (this->height() - this->currentImage.height()) / 2;
    painter.drawImage(x, y, this->currentImage);
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
  auto text = QString("FPS: %1\n").arg(this->currentFps);
  if (this->playbackController && this->showDebugInfo)
    text += this->playbackController->getStatus();
  auto textSize = QFontMetrics(painter.font()).size(0, text);

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

  QRect graphRect;
  graphRect.setSize(QSize(500, 300));
  graphRect.moveBottomLeft(QPoint(0, this->height()));

  painter.setBrush(Qt::white);
  painter.drawRect(graphRect);

  constexpr unsigned blockDistance = 5 + 2;
  QRect frameRect;
  frameRect.setSize(QSize(5, 5));
  frameRect.moveBottom(graphRect.bottom() - 5);

  const auto colorMap =
        std::map<FrameState, QColor>({{FrameState::DownloadQueued, Qt::cyan},
                                           {FrameState::Downloaded, Qt::blue},
                                           {FrameState::Decoded, Qt::yellow},
                                           {FrameState::Converted, Qt::green}});

  const auto leftStart = graphRect.left() + 5;
  auto info = this->playbackController->getFrameQueueInfo();
  for (size_t i = 0; i < info.size(); i++)
  {
    frameRect.moveLeft(leftStart + int(blockDistance * i));
    
    if (info[i].isBeingProcessed)
      painter.setPen(Qt::black);
    else
      painter.setPen(Qt::NoPen);
    
    painter.setBrush(colorMap.at(info[i].frameState));

    painter.drawRect(frameRect);
  }
}

void ViewWidget::clearMessages()
{
  this->messages.clear();
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

void ViewWidget::timerEvent(QTimerEvent *event)
{
  if (event && event->timerId() != timer.timerId())
    return QWidget::timerEvent(event);
  if (this->playbackController == nullptr)
    return;
  auto frameBuffer = this->playbackController->getFrameConversionBuffer();
  if (frameBuffer == nullptr)
    return;

  if (auto nextImage = frameBuffer->getNextImage())
  {
    DEBUG("Timer even. Got next image");
    this->currentImage = *nextImage;
  }
  else
  {
    DEBUG("Timer even. No new image available.");
    return;
  }

  // Update the FPS counter every 50 frames
  this->timerFPSCounter++;
  if (this->timerFPSCounter >= 50)
  {
    auto   newFrameTime         = QTime::currentTime();
    double msecsSinceLastUpdate = (double)this->timerLastFPSTime.msecsTo(newFrameTime);

    // Print the frames per second as float with one digit after the decimal dot.
    this->currentFps = (50.0 / (msecsSinceLastUpdate / 1000.0));

    this->timerLastFPSTime = QTime::currentTime();
    this->timerFPSCounter  = 0;
  }

  this->update();
}
