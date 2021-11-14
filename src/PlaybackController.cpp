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
  this->segmentBuffer->abort();
}

void PlaybackController::reset()
{
  if (this->decoder)
    this->decoder->abort();
  if (this->parser)
    this->parser->abort();
  if (this->conversion)
    this->conversion->abort();
  if (this->segmentBuffer)
    this->segmentBuffer->abort();
  this->downloader.reset(nullptr);
  this->parser.reset(nullptr);
  this->conversion.reset(nullptr);
  this->decoder.reset(nullptr);
  this->segmentBuffer.reset(nullptr);

  this->logger->clearMessages();

  this->segmentBuffer = std::make_unique<SegmentBuffer>();
  this->downloader    = std::make_unique<FileDownloader>(this->logger);
  this->parser        = std::make_unique<FileParserThread>(this->logger, this->segmentBuffer.get());
  this->conversion =
      std::make_unique<FrameConversionThread>(this->logger, this->segmentBuffer.get());
  this->decoder = std::make_unique<DecoderThread>(this->logger, this->segmentBuffer.get());

  connect(this->downloader.get(),
          &FileDownloader::downloadOfSegmentFinished,
          this,
          &PlaybackController::downloadOfSegmentFinished);
  connect(this->segmentBuffer.get(),
          &SegmentBuffer::segmentRemovedFromBuffer,
          this,
          &PlaybackController::fillDownloadQueue);

  this->logger->addMessage("Playback Controller initialized", LoggingPriority::Info);
}

bool PlaybackController::openJsonManifestFile(QString jsonManifestFile)
{
  this->manifestFile = std::make_unique<ManifestFile>(this->logger);
  auto success       = this->manifestFile->openJsonManifestFile(jsonManifestFile);
  if (success)
  {
    this->activateManifest();
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
    this->activateManifest();
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

void PlaybackController::activateManifest()
{
  if (this->manifestFile->isopenGopAdaptiveResolutionChange())
  {
    this->highestRenditionFirstSegment = std::make_unique<Segment>();
    this->highestRenditionFirstSegment->segmentInfo =
        this->manifestFile->getSegmentSPSHighestRendition();
    this->downloader->addFileToDownloadQueue(this->highestRenditionFirstSegment.get());
  }
  this->fillDownloadQueue();
}

void PlaybackController::downloadOfSegmentFinished()
{
  if (this->highestRenditionFirstSegment)
  {
    if (this->highestRenditionFirstSegment->compressedData.isEmpty())
      this->logger->addMessage("Recieved no data for highest rendition segment",
                               LoggingPriority::Error);
    this->highestRenditionFirstSegment.reset();
  }
  else
    this->segmentBuffer->onDownloadOfSegmentFinished();
}

void PlaybackController::fillDownloadQueue()
{
  while (this->segmentBuffer->getNrOfBufferedSegments() <
         this->manifestFile->getMaxSegmentBufferSize())
  {
    auto segment         = this->segmentBuffer->getNextDownloadSegment();
    segment->segmentInfo = this->manifestFile->getNextSegmentInfo();
    this->downloader->addFileToDownloadQueue(segment);
  }
}
