/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QAction>
#include <QActionGroup>
#include <QDesktopServices>
#include <QMainWindow>
#include <QMouseEvent>
#include <QSettings>

#include "PlaybackController.h"
#include "ui_MainWindows.h"

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

  void openLocalFolder();
  void toggleFullscreen(bool checked);
  void toggleShowDebug(bool checked);
  void toggleShowProgressGraph(bool checked);
  void onSelectVVDeCLibrary();
  void openFixedFile();
  void openFixedUrl();

private:
  Ui::MainWindow ui;

  virtual void keyPressEvent(QKeyEvent *event) override;

private:
  void                         createMenusAndActions();
  QAction                      actionFullScreen;
  QAction                      actionShowThreadStatus;
  QAction                      actionShowProgressGraph;
  QScopedPointer<QActionGroup> actionGroup;

  QPointer<QAction> fixedFileActions[1];
  QPointer<QAction> fixedURLActions[1];

  bool wasMaximizedBeforeFullScreen{false};

  std::unique_ptr<PlaybackController> playbackController;
};
