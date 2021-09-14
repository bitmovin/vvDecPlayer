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
  this->parser->abort();
  this->conversion->abort();
  this->segmentBuffer.abort();
}

void PlaybackController::reset()
{
  this->downloader = std::make_unique<FileDownloader>(this->logger, &this->segmentBuffer);
  this->parser     = std::make_unique<FileParserThread>(this->logger, &this->segmentBuffer);
  this->conversion = std::make_unique<FrameConversionThread>(this->logger, &this->segmentBuffer);
  this->decoder    = std::make_unique<DecoderThread>(this->logger, &this->segmentBuffer);

  this->manifestFile = std::make_unique<ManifestFile>(this->logger);

  this->logger->clearMessages();
  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

bool PlaybackController::openJsonManifestFile(QString jsonManifestFile)
{
  auto success = this->manifestFile->openJsonManifestFile(jsonManifestFile);
  if (success)
    this->downloader->activateManifest(this->manifestFile.get());
  return success;
}

bool PlaybackController::openPredefinedManifest(unsigned predefinedManifestID)
{
  auto success = this->manifestFile->openPredefinedManifest(predefinedManifestID);
  if (success)
    this->downloader->activateManifest(this->manifestFile.get());
  return success;
}

void PlaybackController::gotoSegment(unsigned segmentNumber)
{
  if (!this->manifestFile)
    return;
  this->manifestFile->gotoSegment(segmentNumber);
}

QString PlaybackController::getStatus()
{
  QString status;
  status += "Downloader: " + this->downloader->getStatus() + "\n";
  status += "Parser: " + this->parser->getStatus() + "\n";
  status += "Decoder: " + this->decoder->getStatus() + "\n";
  status += "Conversion: " + this->conversion->getStatus() + "\n";
  return status;
}
