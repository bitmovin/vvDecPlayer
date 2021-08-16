/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"

#include "FrameConversionBuffer.h"
#include <QBasicTimer>
#include <QImage>
#include <QTime>
#include <QWidget>
#include <chrono>
#include <mutex>

class ViewWidget : public QWidget, public ILogger
{
  Q_OBJECT;

public:
  ViewWidget(QWidget *parent);

  void setFrameConversionBuffer(FrameConversionBuffer *frameConversionBuffer);

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
  void drawFps(QPainter &painter);

  QBasicTimer  timer;
  int          timerFPSCounter{};
  QTime        timerLastFPSTime;
  double       currentFps{};
  virtual void timerEvent(QTimerEvent *event) override;

  FrameConversionBuffer *frameConversionBuffer{};
  QImage                 currentImage;
};
