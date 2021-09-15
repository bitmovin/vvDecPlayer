/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <PlaybackController.h>
#include <common/ILogger.h>

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

  void setPlaybackController(PlaybackController *playbackController);

  void addMessage(QString message, LoggingPriority priority) override;
  void clearMessages() override;
  void setScaleVideo(bool scaleVideo);
  void setShowDebugInfo(bool showDebugInfo);
  void setShowProgressGraph(bool drawGraph);

  void setPlaybackFps(double framerate);
  void onPlayPause();
  void onStep();

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
  void drawFPSAndStatusText(QPainter &painter);
  void drawProgressGraph(QPainter &painter);

  QBasicTimer  timer;
  int          timerFPSCounter{};
  QTime        timerLastFPSTime;
  double       targetFPS{};
  double       actualFPS{};
  bool         pause{false};
  virtual void timerEvent(QTimerEvent *event) override;

  void getNextFrame();

  PlaybackController *         playbackController{};
  SegmentBuffer::FrameIterator curFrame;

  int debugInfoRenderMaxWidth{};

  unsigned frameSegmentOffset{};

  bool scaleVideo{false};
  bool showDebugInfo{false};
  bool showProgressGraph{false};
};
