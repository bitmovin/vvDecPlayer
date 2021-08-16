/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "ViewWidget.h"

#include <QPainter>

namespace
{

constexpr auto INFO_MESSAGE_TIMEOUT = std::chrono::seconds(10);

}

ViewWidget::ViewWidget(QWidget *parent) : QWidget(parent) {}

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

void ViewWidget::clearMessages()
{
  this->messages.clear();
  this->update();
}
