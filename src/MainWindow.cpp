/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#include "Mainwindow.h"

#include "typedef.h"

#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  this->ui.setupUi(this);
  this->setFocusPolicy(Qt::StrongFocus);
  this->createMenusAndActions();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  // qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz")<<"Key: "<< event;

  int  key         = event->key();
  bool controlOnly = (event->modifiers() == Qt::ControlModifier);

  if (key == Qt::Key_Escape)
  {
    if (isFullScreen())
    {

      this->actionFullScreen.trigger();
      return;
    }
  }
  else if (key == Qt::Key_F && controlOnly)
  {
    this->actionFullScreen.trigger();
    return;
  }
  else if (key == Qt::Key_Space)
  {
    // ui.playbackController->on_playPauseButton_clicked();
    return;
  }

  QWidget::keyPressEvent(event);
}

void MainWindow::toggleFullscreen()
{
  // Single window mode. Hide/show all panels and set/restore the main window
  // to/from fullscreen.

  if (isFullScreen())
  {
    // We are in full screen mode. Toggle back to windowed mode.

    // Show all dock panels which were previously visible.
    // if (panelsVisible[0])
    //   ui.propertiesDock->show();

    if (!is_Q_OS_MAC)
      ui.menuBar->show();

    // Show the window normal or maximized (depending on how it was shown
    // before)
    if (this->wasMaximizedBeforeFullScreen)
      this->showMaximized();
    else
      this->showNormal();
  }
  else
  {
    // Toggle to full screen mode. Save which panels are currently visible. This
    // is restored when returning from full screen.
    // panelsVisible[0] = ui.propertiesDock->isVisible();

    // Hide panels
    // ui.propertiesDock->hide();

    if (!is_Q_OS_MAC)
      ui.menuBar->hide();

    this->wasMaximizedBeforeFullScreen = this->isMaximized();
    this->showFullScreen();
  }
}

void MainWindow::createMenusAndActions()
{
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction("Exit", this, &MainWindow::close);

  auto viewMenu = this->ui.menuBar->addMenu("View");

  const bool menuActionsNoteCreatedYet = this->actionGroup.isNull();
  Q_ASSERT_X(
      menuActionsNoteCreatedYet, Q_FUNC_INFO, "Only call this initialization function once.");

  auto configureCheckableAction = [this](QAction &     action,
                                         QActionGroup *actionGroup,
                                         QString       text,
                                         bool          checked,
                                         void (MainWindow::*func)(),
                                         const QKeySequence &shortcut  = {},
                                         bool                isEnabled = true) {
    action.setParent(this);
    action.setCheckable(true);
    action.setChecked(checked);
    action.setText(text);
    action.setShortcut(shortcut);
    if (actionGroup)
      actionGroup->addAction(&action);
    if (!isEnabled)
      action.setEnabled(false);
    connect(&action, &QAction::triggered, this, func);
  };

  this->actionGroup.reset(new QActionGroup(this));

  configureCheckableAction(this->actionFullScreen,
                           nullptr,
                           "&Fullscreen Mode",
                           false,
                           &MainWindow::toggleFullscreen,
                           Qt::CTRL | Qt::Key_F);
  viewMenu->addAction(&this->actionFullScreen);
}
