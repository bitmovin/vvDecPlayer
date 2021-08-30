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

PlaybackController::~PlaybackController()
{
  this->decoder->abort();
  this->conversion->abort();
  this->segmentBuffer.abort();
}

void PlaybackController::reset()
{
  this->downloader = std::make_unique<FileDownloader>(this->logger, &this->segmentBuffer);
  this->conversion = std::make_unique<FrameConversionThread>(this->logger, &this->segmentBuffer);
  this->decoder    = std::make_unique<DecoderManager>(this->logger, &this->segmentBuffer);

  this->logger->clearMessages();
  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

void PlaybackController::openDirectory(QDir path, QString segmentPattern)
{
  this->downloader->openDirectory(path, segmentPattern);
}

void PlaybackController::openURL(QString url, QString segmentPattern, unsigned segmentNrMax)
{
  this->downloader->openURL(url, segmentPattern, segmentNrMax);
}

QString PlaybackController::getStatus()
{
  QString status;
  status += "Downloader: " + this->downloader->getStatus() + "\n";
  status += "Decoder: " + this->decoder->getStatus() + "\n";
  status += "Conversion: " + this->conversion->getStatus() + "\n";
  return status;
}
