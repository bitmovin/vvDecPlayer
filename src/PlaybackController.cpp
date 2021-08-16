/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

#include <decoder/decoderVVDec.h>

PlaybackController::PlaybackController(ILogger *logger) : logger(logger)
{
  assert(logger != nullptr);
  this->reset();
}

void PlaybackController::reset()
{
  this->fileDownloadManager = std::make_unique<FileDownloadManager>(this->logger);

  this->logger->clearMessages();

  this->decoder = std::make_unique<decoder::decoderVVDec>();
  if (this->decoder->errorInDecoder())
  {
    this->logger->addMessage("Error in decoder: " + this->decoder->decoderErrorString(),
                             LoggingPriority::Error);
  }

  connect(this->fileDownloadManager.get(),
          &FileDownloadManager::onSegmentReadyForDecode,
          this,
          &PlaybackController::onSegmentReadyForDecode);

  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

void PlaybackController::openDirectory(QDir path, QString segmentPattern)
{
  this->fileDownloadManager->openDirectory(path, segmentPattern);
}

void PlaybackController::onSegmentReadyForDecode() {}