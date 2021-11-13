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

#include "DecoderThread.h"

#include <common/functions.h>
#include <decoder/decoderVVDec.h>
#include <parser/VVC/nal_unit_header.h>
#include <parser/common/SubByteReaderLogging.h>

#include <QDebug>
#include <chrono>

#define DEBUG_DECODER_MANAGER 0
#if DEBUG_DECODER_MANAGER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

bool isSPSNAL(QByteArray nalData)
{
  auto data = convertToByteVector(nalData);

  // Skip the NAL unit header
  int readOffset = 0;
  if (data.size() > 3 && data.at(0) == (char)0 && data.at(1) == (char)0 && data.at(2) == (char)1)
    readOffset = 3;
  else if (data.size() > 4 && data.at(0) == (char)0 && data.at(1) == (char)0 &&
           data.at(2) == (char)0 && data.at(3) == (char)1)
    readOffset = 4;

  parser::reader::SubByteReaderLogging reader(data, {}, "", readOffset);
  parser::vvc::nal_unit_header         header;
  header.parse(reader);

  return header.nal_unit_type == parser::vvc::NalType::SPS_NUT;
}

} // namespace

DecoderThread::DecoderThread(ILogger *logger, SegmentBuffer *segmentBuffer)
    : logger(logger), segmentBuffer(segmentBuffer)
{
  this->decoder = std::make_unique<decoder::decoderVVDec>();
  if (this->decoder->errorInDecoder())
  {
    this->logger->addMessage("Error in decoder: " + this->decoder->decoderErrorString(),
                             LoggingPriority::Error);
    return;
  }

  this->decoderThread = std::thread(&DecoderThread::runDecoder, this);
}

DecoderThread::~DecoderThread()
{
  this->abort();
  if (this->decoderThread.joinable())
    this->decoderThread.join();
}

void DecoderThread::abort() { this->decoderAbort = true; }

void DecoderThread::setOpenGopAdaptiveResolutionChange(bool adaptiveResolutioChange)
{
  this->adaptiveResolutioChange = adaptiveResolutioChange;
}

QString DecoderThread::getStatus() const
{
  return (this->decoderAbort ? "Abort " : "") + this->statusText;
}

void DecoderThread::onDownloadOfFirstSPSSegmentFinished(QByteArray segmentData)
{
  auto startPos = findNextNalInData(segmentData, 0);
  if (!startPos)
  {
    this->logger->addMessage("SPS could not be extracted from highest rendition",
                             LoggingPriority::Error);
    return;
  }

  size_t currentDataOffset = *startPos;
  while (currentDataOffset < size_t(segmentData.size()))
  {
    QByteArray nalData;
    if (auto nextNalStart = findNextNalInData(segmentData, currentDataOffset + 3))
    {
      auto length       = *nextNalStart - currentDataOffset;
      nalData           = segmentData.mid(currentDataOffset, length);
      currentDataOffset = *nextNalStart;
    }
    else if (currentDataOffset < size_t(segmentData.size()))
    {
      nalData           = segmentData.mid(currentDataOffset);
      currentDataOffset = segmentData.size();
    }

    if (isSPSNAL(nalData))
    {
      this->highestRenditionSPS = nalData;
      return;
    }
  }

  this->logger->addMessage("SPS could not be extracted from highest rendition",
                           LoggingPriority::Error);
}

void DecoderThread::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  size_t                currentDataOffset        = 0;
  unsigned              currentFrameIdxInSegment = 0;
  auto                  itSegmentData            = this->segmentBuffer->getFirstSegmentToDecode();
  std::queue<Segment *> nextSegmentFrames;
  nextSegmentFrames.push(itSegmentData);

  while (!this->decoderAbort)
  {
    const auto &data                     = itSegmentData->compressedData;
    bool        resetDecoderAfterSegment = false;

    if (auto firstPos = findNextNalInData(data, 0))
      currentDataOffset = *firstPos;
    while (!this->decoderAbort)
    {
      auto state = this->decoder->state();

      if (state == decoder::DecoderState::NeedsMoreData)
      {
        QByteArray nalData;
        if (auto nextNalStart = findNextNalInData(data, currentDataOffset + 3))
        {
          auto length       = *nextNalStart - currentDataOffset;
          nalData           = data.mid(currentDataOffset, length);
          currentDataOffset = *nextNalStart;
        }
        else if (currentDataOffset < size_t(data.size()))
        {
          nalData           = data.mid(currentDataOffset);
          currentDataOffset = data.size();
        }

        if (nalData.isEmpty())
        {
          DEBUG("No more data. Will continue with next segment.");

          // This may block until another segment to decode is available
          this->statusText = "Waiting";
          auto nextSegment = this->segmentBuffer->getNextSegmentToDecode(itSegmentData);
          this->statusText = "Decoding";

          if (!nextSegment)
          {
            this->logger->addMessage("Got no next semgent.", LoggingPriority::Error);
            DEBUG("Got no next segment. Exit decoder thread.");
            return;
          }

          DEBUG(QString("Got next segment Rendition %1 Segment %2")
                    .arg(nextSegment->segmentInfo.rendition)
                    .arg(nextSegment->segmentInfo.segmentNumber));

          auto renditionSwitch =
              nextSegment->segmentInfo.rendition != itSegmentData->segmentInfo.rendition;

          itSegmentData = nextSegment;
          nextSegmentFrames.push(nextSegment);

          if (renditionSwitch && !this->adaptiveResolutioChange)
          {
            resetDecoderAfterSegment = true;
            DEBUG("Pushing empty data (EOF)");
            if (!this->decoder->pushData(nalData))
            {
              this->logger->addMessage("Error pushing empty data (EOF)", LoggingPriority::Error);
              break;
            }
          }
          else
            break;
        }
        else
        {
          if (!this->highestRenditionSPS.isEmpty() && isSPSNAL(nalData))
          {
            DEBUG("Replace SPS with SPS from highest rendition");
            nalData = this->highestRenditionSPS;
          }

          DEBUG("Pushing " << nalData.size() << " bytes");
          if (!this->decoder->pushData(nalData))
          {
            this->logger->addMessage("Error pushing data", LoggingPriority::Error);
            break;
          }
        }
      }

      if (state == decoder::DecoderState::RetrieveFrames)
      {
        DEBUG("Checking for next frame ");
        if (this->decoder->decodeNextFrame())
        {
          auto itSegmentFrames = nextSegmentFrames.front();
          if (currentFrameIdxInSegment >= itSegmentFrames->frames.size())
          {
            nextSegmentFrames.pop();
            if (nextSegmentFrames.empty())
            {
              this->logger->addMessage(
                  QString(
                      "Error putting frame %1 into buffer. Got more frames then there should be.")
                      .arg(currentFrameIdxInSegment),
                  LoggingPriority::Error);
              break;
            }
            else
            {
              itSegmentFrames          = nextSegmentFrames.front();
              currentFrameIdxInSegment = 0;
              if (itSegmentFrames->frames.empty())
              {
                this->logger->addMessage(QString("Next segment has no frames"),
                                         LoggingPriority::Error);
                break;
              }
            }
          }

          auto &frame       = itSegmentFrames->frames.at(currentFrameIdxInSegment);
          frame->rawYUVData  = this->decoder->getRawFrameData();
          frame->pixelFormat = this->decoder->getYUVPixelFormat();
          frame->frameSize   = this->decoder->getFrameSize();
          frame->frameState  = FrameState::Decoded;

          DEBUG(QString("Saving frame (%1x%2) into frame idx %3 segment %4 rendition %5")
                    .arg(frame->frameSize.width)
                    .arg(frame->frameSize.height)
                    .arg(currentFrameIdxInSegment)
                    .arg(itSegmentFrames->segmentInfo.segmentNumber)
                    .arg(itSegmentFrames->segmentInfo.rendition));
          this->segmentBuffer->onFrameDecoded();
          currentFrameIdxInSegment++;
        }
      }

      if (state == decoder::DecoderState::Error)
      {
        DEBUG("Decoding error");
        this->logger->addMessage(QString("Error decoding rend %1 seg %2 frame %3")
                                     .arg(itSegmentData->segmentInfo.rendition)
                                     .arg(itSegmentData->segmentInfo.segmentNumber)
                                     .arg(currentFrameIdxInSegment),
                                 LoggingPriority::Error);
        break;
      }

      if (state == decoder::DecoderState::EndOfBitstream)
      {
        DEBUG("Decoding EOF");
        break;
      }
    }

    if (this->decoderAbort)
      return;

    DEBUG("Start decoding of next segment");
    if (resetDecoderAfterSegment)
    {
      DEBUG("Reset decoder");
      this->decoder->resetDecoder();
    }
  }
}
