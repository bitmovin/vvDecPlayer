/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "ViewWidget.h"

#include <QPainter>
#include <QTimerEvent>

#define DEBUG_WIDGET 1
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
  double frameRate     = 24.0;
  auto   timerInterval = int(1000.0 / frameRate);
  timer.start(timerInterval, Qt::PreciseTimer, this);
}

void ViewWidget::setFrameConversionBuffer(FrameConversionBuffer *frameConversionBuffer)
{
  assert(frameConversionBuffer != nullptr);
  this->frameConversionBuffer = frameConversionBuffer;
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

  // Get the full size of the area that we can draw on (from the paint device base)
  QPoint drawArea_botR(width(), height());

  if (!this->currentImage.isNull())
  {
    painter.drawImage(0, 0, this->currentImage);
  }

  this->drawFps(painter);
  this->drawAndUpdateMessages(painter);
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
    painter.fillRect(boxRect, colorMap.at(it->priority));
    painter.drawRect(boxRect);

    painter.drawText(textRect, Qt::AlignCenter, text);

    yOffset += boxRect.height();
    it++;
  }
}

void ViewWidget::drawFps(QPainter &painter)
{
  auto text     = QString("%1").arg(this->currentFps);
  auto textSize = QFontMetrics(painter.font()).size(0, text);

  QRect textRect;
  textRect.setSize(textSize);
  textRect.moveTopRight(QPoint(this->width(), 0));

  auto boxRect = textRect + QMargins(5, 5, 5, 5);
  painter.drawText(textRect, Qt::AlignCenter, text);
}

void ViewWidget::clearMessages()
{
  this->messages.clear();
  this->update();
}

void ViewWidget::timerEvent(QTimerEvent *event)
{
  if (event && event->timerId() != timer.timerId())
    return QWidget::timerEvent(event);
  if (this->frameConversionBuffer == nullptr)
    return;

  if (auto nextImage = this->frameConversionBuffer->getNextImage())
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
