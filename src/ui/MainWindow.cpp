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

#include "MainWindow.h"

#include <common/typedef.h>
#include <decoder/decoderVVDec.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>

constexpr auto DEFAULT_SEGMENT_PATTERN = "segment-%i.vvc";
constexpr auto SINTEL_SEGMENT_NR       = 887;

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

  int key = event->key();
  if (key == Qt::Key_Escape)
  {
    if (isFullScreen())
    {
      this->actionFullScreen.trigger();
      return;
    }
  }
  else if (key == Qt::Key_Space)
  {
    ui.viewWidget->onPlayPause();
    return;
  }
  else if (key == Qt::Key_Right)
  {
    ui.viewWidget->onStep();
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

void MainWindow::toggleScaleVideo(bool checked) { this->ui.viewWidget->setScaleVideo(checked); }

void MainWindow::toggleShowDebug(bool checked) { this->ui.viewWidget->setShowDebugInfo(checked); }

void MainWindow::toggleShowProgressGraph(bool checked)
{
  this->ui.viewWidget->setShowProgressGraph(checked);
}

void MainWindow::createMenusAndActions()
{
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction("Open JSON manifest ...", this, &MainWindow::openJsonManifestFile);
  fileMenu->addSeparator();

  auto presetURLs = fileMenu->addMenu("Bitmovin Streams");
  for (int i = 0; i < 1; i++)
  {
    this->fixedURLActions[i] = new QAction(this);
    this->fixedURLActions[i]->setText("Sintel AWS S3 720p");
    this->fixedURLActions[i]->setData(0);
    connect(this->fixedURLActions[i].data(), &QAction::triggered, this, &MainWindow::openFixedUrl);
    presetURLs->addAction(this->fixedURLActions[i]);
  }

  fileMenu->addSeparator();
  fileMenu->addAction("Exit", this, &MainWindow::close);

  const bool menuActionsNoteCreatedYet = this->actionGroup.isNull();
  Q_ASSERT_X(
      menuActionsNoteCreatedYet, Q_FUNC_INFO, "Only call this initialization function once.");

  auto configureCheckableAction = [this](QAction &     action,
                                         QActionGroup *actionGroup,
                                         QWidget *     addToWidget,
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
    this->addAction(&action);
    if (addToWidget)
      addToWidget->addAction(&action);
    connect(&action, &QAction::triggered, this, func);
  };

  this->actionGroup.reset(new QActionGroup(this));

  auto viewMenu = this->ui.menuBar->addMenu("View");
  configureCheckableAction(this->actionFullScreen,
                           nullptr,
                           viewMenu,
                           "&Fullscreen Mode",
                           false,
                           &MainWindow::toggleFullscreen,
                           Qt::CTRL | Qt::Key_F);
  configureCheckableAction(this->actionScaleVideo,
                           nullptr,
                           viewMenu,
                           "&Scale Video",
                           true,
                           &MainWindow::toggleScaleVideo,
                           Qt::CTRL | Qt::Key_S);
  configureCheckableAction(this->actionShowThreadStatus,
                           nullptr,
                           viewMenu,
                           "Show &Debug",
                           false,
                           &MainWindow::toggleShowDebug,
                           Qt::CTRL | Qt::Key_D);
  configureCheckableAction(this->actionShowProgressGraph,
                           nullptr,
                           viewMenu,
                           "Show &Progress Graph",
                           false,
                           &MainWindow::toggleShowProgressGraph,
                           Qt::CTRL | Qt::Key_P);

  auto playbackMenu = this->ui.menuBar->addMenu("Playback");
  this->addAction(playbackMenu->addAction(
      "Goto segment number ...", this, &MainWindow::onGotoSegmentNumber, Qt::CTRL | Qt::Key_G));
  this->addAction(playbackMenu->addAction(
      "Increase rendition", this, &MainWindow::onIncreaseRendition, Qt::Key_Up));
  this->addAction(playbackMenu->addAction(
      "Decrease rendition", this, &MainWindow::onDecreaseRendition, Qt::Key_Down));

  auto settingsMenu = this->ui.menuBar->addMenu("Settings");
  settingsMenu->addAction("Select VVdeC library ...", this, &MainWindow::onSelectVVDeCLibrary);
}

void MainWindow::openJsonManifestFile()
{
  QFileDialog fileDialog(this, "Select directory with VVC segments", "", "Manifest (*.json)");

  if (fileDialog.exec())
  {
    auto files = fileDialog.selectedFiles();
    if (files.size() == 0)
      return;
    auto file = files[0];

    if (this->playbackController->openJsonManifestFile(files[0]))
    {
      auto manifest = this->playbackController->getManifest();
      auto renditionInfo = manifest->getCurrentRenditionInfo();
      this->ui.viewWidget->setPlaybackFps(renditionInfo->fps);
      this->ui.viewWidget->setPlotMaxBitrate(manifest->getPlotMaxBitrate());
    }
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

void MainWindow::openFixedUrl()
{
  auto action = qobject_cast<QAction *>(sender());
  if (action)
  {
    auto id = action->data().toInt();
    this->playbackController->openPredefinedManifest(id);
    auto manifest = this->playbackController->getManifest();
    auto renditionInfo = manifest->getCurrentRenditionInfo();
    this->ui.viewWidget->setPlaybackFps(renditionInfo->fps);
    this->ui.viewWidget->setPlotMaxBitrate(manifest->getPlotMaxBitrate());
  }
}

void MainWindow::onGotoSegmentNumber()
{
  bool ok            = false;
  auto max           = 2147483647;
  auto segmentNumber = QInputDialog::getInt(
      this, "Goto segment number", "Please provide a segment number", 0, 0, max, 1, &ok);
  if (!ok)
    return;
  this->playbackController->gotoSegment(segmentNumber);
}

void MainWindow::onIncreaseRendition() { this->playbackController->increaseRendition(); }

void MainWindow::onDecreaseRendition() { this->playbackController->decreaseRendition(); }
