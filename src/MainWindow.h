/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

#include <QAction>
#include <QDesktopServices>
#include <QMainWindow>
#include <QSettings>
#include <QMouseEvent>
#include <QActionGroup>

#include "ui_MainWindows.h"
#include "PlaybackController.h"

class QAction;
class playlistItem;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override {
    this->actionFullScreen.trigger();
    event->accept();
  }

private slots:

  void toggleFullscreen();

private:
  Ui::MainWindow ui;

  virtual void keyPressEvent(QKeyEvent *event) override;

private:
  void createMenusAndActions();
  QAction actionFullScreen;
  QScopedPointer<QActionGroup> actionGroup;

  bool wasMaximizedBeforeFullScreen {false};

  std::unique_ptr<PlaybackController> playbackController;
};
