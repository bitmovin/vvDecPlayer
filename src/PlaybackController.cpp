/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

#include <QDebug>
#include <decoder/decoderVVDec.h>

PlaybackController::PlaybackController(ILogger *logger)
    : logger(logger)
{
  assert(logger != nullptr);
  this->reset();
}

PlaybackController::~PlaybackController()
{
  this->frameConversionBuffer->abort();
  this->decoderManager->abort();
  this->fileDownloadManager->abort();
}

void PlaybackController::reset()
{
  this->frameConversionBuffer = std::make_unique<FrameConversionBuffer>();
  this->fileDownloadManager   = std::make_unique<FileDownloadManager>(this->logger);
  this->decoderManager =
      std::make_unique<DecoderManager>(this->logger, this->frameConversionBuffer.get());

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

QString PlaybackController::getStatus()
{
  QString status;
  status += this->fileDownloadManager->getStatus() + "\n";
  status += this->decoderManager->getStatus() + "\n";
  status += this->frameConversionBuffer->getStatus() + "\n";
  return status;
}

void PlaybackController::onSegmentReadyForDecode()
{
  if (!this->decoderManager->isDecodeRunning())
    if (auto file = this->fileDownloadManager->getNextDownloadedFile())
      this->decoderManager->decodeFile(file);
}

void PlaybackController::onDecodeOfSegmentDone()
{
  if (auto file = this->fileDownloadManager->getNextDownloadedFile())
    this->decoderManager->decodeFile(file);
}
