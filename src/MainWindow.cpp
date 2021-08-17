/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "Mainwindow.h"

#include <common/typedef.h>
#include <decoder/decoderVVDec.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  this->ui.setupUi(this);
  this->setFocusPolicy(Qt::StrongFocus);
  this->createMenusAndActions();

  this->playbackController = std::make_unique<PlaybackController>(this->ui.viewWidget);
  this->ui.viewWidget->setPlaybackController(this->playbackController.get());
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
  else if (key == Qt::Key_D && controlOnly)
  {
    this->actionShowThreadStatus.trigger();
    return;
  }
  else if (key == Qt::Key_P && controlOnly)
  {
    this->actionShowProgressGraph.trigger();
    return;
  }
  else if (key == Qt::Key_Space)
  {
    // ui.playbackController->on_playPauseButton_clicked();
    return;
  }

  QWidget::keyPressEvent(event);
}

void MainWindow::toggleFullscreen(bool)
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

void MainWindow::toggleShowDebug(bool checked) { this->ui.viewWidget->setShowDebugInfo(checked); }

void MainWindow::toggleShowProgressGraph(bool checked)
{
  this->ui.viewWidget->setShowProgressGraph(checked);
}

void MainWindow::createMenusAndActions()
{
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction("Open local folder ...", this, &MainWindow::openLocalFolder);
  fileMenu->addSeparator();
  fileMenu->addAction("Exit", this, &MainWindow::close);

  const bool menuActionsNoteCreatedYet = this->actionGroup.isNull();
  Q_ASSERT_X(
      menuActionsNoteCreatedYet, Q_FUNC_INFO, "Only call this initialization function once.");

  auto configureCheckableAction = [this](QAction &     action,
                                         QActionGroup *actionGroup,
                                         QString       text,
                                         bool          checked,
                                         void (MainWindow::*func)(bool),
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

  auto viewMenu = this->ui.menuBar->addMenu("View");
  configureCheckableAction(this->actionFullScreen,
                           nullptr,
                           "&Fullscreen Mode",
                           false,
                           &MainWindow::toggleFullscreen,
                           Qt::CTRL | Qt::Key_F);
  viewMenu->addAction(&this->actionFullScreen);
  configureCheckableAction(this->actionShowThreadStatus,
                           nullptr,
                           "Show &Debug",
                           false,
                           &MainWindow::toggleShowDebug,
                           Qt::CTRL | Qt::Key_D);
  viewMenu->addAction(&this->actionShowThreadStatus);
  configureCheckableAction(this->actionShowProgressGraph,
                           nullptr,
                           "Show &Progress Graph",
                           false,
                           &MainWindow::toggleShowProgressGraph,
                           Qt::CTRL | Qt::Key_P);
  viewMenu->addAction(&this->actionShowProgressGraph);

  auto settingsMenu = this->ui.menuBar->addMenu("Settings");
  settingsMenu->addAction("Select VVdeC library ...", this, &MainWindow::onSelectVVDeCLibrary);
}

void MainWindow::openLocalFolder()
{
  QFileDialog fileDialog(this, "Select directory with VVC segments");
  fileDialog.setDirectory(QDir::current());
  fileDialog.setFileMode(QFileDialog::Directory);

  if (fileDialog.exec())
  {
    auto files = fileDialog.selectedFiles();
    if (files.size() == 0)
      return;
    auto path = files[0];

    bool ok             = false;
    auto segmentPattern = QInputDialog::getText(this,
                                                "Segment name pattern",
                                                "Please provide a pattern for the files to read "
                                                "from the directory (e.g. \"segment_%i.vvc\")",
                                                QLineEdit::Normal,
                                                "segment-%i.vvc",
                                                &ok);
    if (!ok)
      return;

    if (!segmentPattern.contains("%i"))
    {
      QMessageBox::critical(
          this,
          "Error in segment name",
          "The selected segment name pattern is invalid. It must contain a \"%i\"");
      return;
    }

    this->playbackController->openDirectory(path, segmentPattern);
  }
}

void MainWindow::onSelectVVDeCLibrary()
{
  QFileDialog fileDialog(this, "Select VVDeC decoder library");
  fileDialog.setDirectory(QDir::current());
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (is_Q_OS_LINUX)
    fileDialog.setNameFilter("Lirary files (*.so.* *.so)");
  if (is_Q_OS_MAC)
    fileDialog.setNameFilter("Library files (*.dylib)");
  if (is_Q_OS_WIN)
    fileDialog.setNameFilter("Library files (*.dll)");

  if (fileDialog.exec())
  {
    auto files = fileDialog.selectedFiles();
    if (files.size() == 0)
      return;
    auto file = files[0];

    QString error;
    if (!decoder::decoderVVDec::checkLibraryFile(file, error))
    {
      QMessageBox::critical(
          this,
          "Error testing the library",
          "The selected file does not appear to be a usable libVVDec decoder library. Error: " +
              error);
      return;
    }

    QSettings settings;
    settings.setValue("libVVDecFile", file);

    this->playbackController->reset();
  }
}
