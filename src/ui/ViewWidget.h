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

#pragma once

#include <PlaybackController.h>
#include <common/ILogger.h>

#include <QBasicTimer>
#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTime>
#include <QWidget>
#include <chrono>
#include <mutex>

class ViewWidget : public QOpenGLWidget, public QOpenGLFunctions, public ILogger
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
  void setPlotMaxBitrate(unsigned plotMaxBitrate);
  void onPlayPause();
  void onStep();

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
  struct ViewWidgetMessage
  {
    std::chrono::time_point<std::chrono::steady_clock> timeAdded;
    QString                                            message;
    LoggingPriority                                    priority;
  };
  std::vector<ViewWidgetMessage> messages;
  std::mutex                     messagesMutex;

  void drawCurrentFrame(QPainter &painter);
  void drawAndUpdateMessages(QPainter &painter);
  void drawFPSAndStatusText(QPainter &painter);
  void drawRenditionInfo(QPainter &painter);
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
  unsigned plotMaxBitrate{1000};

  bool scaleVideo{true};
  bool showDebugInfo{false};
  bool showProgressGraph{false};

  std::unique_ptr<QOpenGLShader>        vertexShader;
  std::unique_ptr<QOpenGLShader>        fragmentShader;
  std::unique_ptr<QOpenGLShaderProgram> shaderProgram;
  int                                   textureUniformY{};
  int                                   textureUniformU{};
  int                                   textureUniformV{};
  std::unique_ptr<QOpenGLTexture>       textureY;
  std::unique_ptr<QOpenGLTexture>       textureU;
  std::unique_ptr<QOpenGLTexture>       textureV;
  GLuint                                textureIdY{};
  GLuint                                textureIdU{};
  GLuint                                textureIdV{};
};
