/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

#include <QDebug>
#include <decoder/decoderVVDec.h>

PlaybackController::PlaybackController(ILogger *logger) : logger(logger)
{
  assert(logger != nullptr);
  this->reset();
}

void PlaybackController::reset()
{
  this->fileDownloadManager = std::make_unique<FileDownloadManager>(this->logger);
  this->decoderManager      = std::make_unique<DecoderManager>(this->logger);

  this->logger->clearMessages();

  connect(this->fileDownloadManager.get(),
          &FileDownloadManager::onSegmentReadyForDecode,
          this,
          &PlaybackController::onSegmentReadyForDecode);

  connect(this->decoderManager.get(),
          &DecoderManager::onDecodeOfSegmentDone,
          this,
          &PlaybackController::onDecodeOfSegmentDone);

  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

void PlaybackController::openDirectory(QDir path, QString segmentPattern)
{
  this->fileDownloadManager->openDirectory(path, segmentPattern);
}

void PlaybackController::onSegmentReadyForDecode()
{
  qDebug() << "onSegmentReadyForDecode";
  if (!this->decoderManager->isDecodeRunning())
    if (auto file = this->fileDownloadManager->getNextDownloadedFile())
      this->decoderManager->decodeFile(file);
}

void PlaybackController::onDecodeOfSegmentDone()
{
  qDebug() << "onDecodeOfSegmentDone";
  if (auto file = this->fileDownloadManager->getNextDownloadedFile())
    this->decoderManager->decodeFile(file);
}
