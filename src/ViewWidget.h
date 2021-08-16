/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"

#include <QWidget>
#include <chrono>
#include <mutex>

class ViewWidget : public QWidget, public ILogger
{
  Q_OBJECT;

public:
  ViewWidget(QWidget *parent);

  void addMessage(QString message, LoggingPriority priority) override;
  void clearMessages() override;

private:
  virtual void paintEvent(QPaintEvent *event) override;

  struct ViewWidgetMessage
  {
    std::chrono::time_point<std::chrono::steady_clock> timeAdded;
    QString                                            message;
    LoggingPriority                                    priority;
  };
  std::vector<ViewWidgetMessage> messages;
  std::mutex                     messagesMutex;

  void drawAndUpdateMessages(QPainter &painter);
};
