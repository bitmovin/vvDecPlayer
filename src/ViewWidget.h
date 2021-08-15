/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QWidget>
#include <chrono>

class ViewWidget : public QWidget
{
  Q_OBJECT;

public:
  ViewWidget(QWidget *parent);

  enum class MessagePriority
  {
    Error,
    Warning,
    Info
  };

  void addMessage(std::string message, MessagePriority priority);

private:
  virtual void paintEvent(QPaintEvent *event) override;

  struct ViewWidgetMessage
  {
    std::chrono::time_point<std::chrono::steady_clock> timeAdded;
    std::string                                        message;
    MessagePriority                                    priority;
  };
  std::vector<ViewWidgetMessage> messages;
  void drawAndUpdateMessages(QPainter &painter);
};
