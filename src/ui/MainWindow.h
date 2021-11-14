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

#include <QAction>
#include <QActionGroup>
#include <QDesktopServices>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPointer>
#include <QSettings>

#include "ui_MainWindows.h"
#include <PlaybackController.h>

class QAction;
class playlistItem;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override
  {
    this->actionFullScreen.trigger();
    event->accept();
  }

private slots:

  void openJsonManifestFile();
  void toggleFullscreen(bool checked);
  void toggleScaleVideo(bool checked);
  void toggleShowDebug(bool checked);
  void toggleShowProgressGraph(bool checked);
  void onSelectVVDeCLibrary();
  void onSelectFFMpegeLibrary();
  void onGotoSegmentNumber();
  void onIncreaseRendition();
  void onDecreaseRendition();
  void openFixedUrl();

private:
  Ui::MainWindow ui;

  virtual void keyPressEvent(QKeyEvent *event) override;

private:
  void                         createMenusAndActions();
  QAction                      actionFullScreen;
  QAction                      actionScaleVideo;
  QAction                      actionShowThreadStatus;
  QAction                      actionShowProgressGraph;
  QScopedPointer<QActionGroup> actionGroup;

  QPointer<QAction> fixedURLActions[1];

  bool wasMaximizedBeforeFullScreen{false};

  std::unique_ptr<PlaybackController> playbackController;
};
