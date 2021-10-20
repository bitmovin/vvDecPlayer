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

#include "PlaybackController.h"

#include <QDebug>
#include <assert.h>
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

  connect(&this->segmentBuffer,
          &SegmentBuffer::startNextDownload,
          this->downloader.get(),
          &FileDownloader::downloadNextFile);
  connect(this->downloader.get(),
          &FileDownloader::downloadOfSegmentFinished,
          &this->segmentBuffer,
          &SegmentBuffer::onDownloadOfSegmentFinished);
  connect(this->downloader.get(),
          &FileDownloader::downloadOfFirstSPSSegmentFinished,
          this->decoder.get(),
          &DecoderThread::onDownloadOfFirstSPSSegmentFinished);

  this->logger->clearMessages();
  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

bool PlaybackController::openJsonManifestFile(QString jsonManifestFile)
{
  this->manifestFile = std::make_unique<ManifestFile>(this->logger);
  auto success       = this->manifestFile->openJsonManifestFile(jsonManifestFile);
  if (success)
  {
    this->downloader->activateManifest(this->manifestFile.get());
    this->decoder->setOpenGopAdaptiveResolutionChange(
        this->manifestFile->isopenGopAdaptiveResolutionChange());
  }
  return success;
}

bool PlaybackController::openPredefinedManifest(unsigned predefinedManifestID)
{
  this->manifestFile = std::make_unique<ManifestFile>(this->logger);
  auto success       = this->manifestFile->openPredefinedManifest(predefinedManifestID);
  if (success)
  {
    this->downloader->activateManifest(this->manifestFile.get());
    this->decoder->setOpenGopAdaptiveResolutionChange(
        this->manifestFile->isopenGopAdaptiveResolutionChange());
  }
  return success;
}

void PlaybackController::gotoSegment(unsigned segmentNumber)
{
  if (!this->manifestFile)
    return;
  this->manifestFile->gotoSegment(segmentNumber);
}

void PlaybackController::increaseRendition() { this->manifestFile->increaseRendition(); }

void PlaybackController::decreaseRendition() { this->manifestFile->decreaseRendition(); }

QString PlaybackController::getStatus()
{
  QString status;
  status += "Downloader: " + this->downloader->getStatus() + "\n";
  status += "Parser: " + this->parser->getStatus() + "\n";
  status += "Decoder: " + this->decoder->getStatus() + "\n";
  status += "Conversion: " + this->conversion->getStatus() + "\n";
  return status;
}
