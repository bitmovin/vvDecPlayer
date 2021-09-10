/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "DecoderThread.h"

#include <common/functions.h>
#include <decoder/decoderVVDec.h>

#include <QDebug>
#include <chrono>

#define DEBUG_DECODER_MANAGER 0
#if DEBUG_DECODER_MANAGER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

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

QString DecoderThread::getStatus() const
{
  return (this->decoderAbort ? "Abort " : "") + this->statusText;
}

void DecoderThread::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  size_t currentDataOffset{};
  auto   segmentIt = this->segmentBuffer->getFirstSegmentToDecode();

  while (!this->decoderAbort)
  {
    // // Simulate decoding
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1000ms);
    const auto &data = segmentIt->compressedData;

    unsigned currentFrameIdxInSegment = 0;
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

        DEBUG("Pushing " << nalData.size() << " bytes");
        if (!this->decoder->pushData(nalData))
        {
          this->logger->addMessage("Error pushing data", LoggingPriority::Error);
          break;
        }
      }

      if (state == decoder::DecoderState::RetrieveFrames)
      {
        DEBUG("Checking for next frame ");
        if (this->decoder->decodeNextFrame())
        {
          if (currentFrameIdxInSegment >= segmentIt->frames.size())
          {
            this->logger->addMessage(QString("Error putting frame %1 into buffer. Frame not found.")
                                         .arg(currentFrameIdxInSegment),
                                     LoggingPriority::Error);
          }

          auto frame         = segmentIt->frames.at(currentFrameIdxInSegment);
          frame->rawYUVData  = this->decoder->getRawFrameData();
          frame->pixelFormat = this->decoder->getYUVPixelFormat();
          frame->frameSize   = this->decoder->getFrameSize();
          frame->frameState  = FrameState::Decoded;

          DEBUG("Retrived frame " << currentFrameIdxInSegment << " with size "
                                  << frame->frameSize.width << "x" << frame->frameSize.height);
          currentFrameIdxInSegment++;
        }
      }

      if (state == decoder::DecoderState::Error)
      {
        DEBUG("Decoding error");
        this->logger->addMessage(QString("Error decoding frame %1").arg(currentFrameIdxInSegment),
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

    // This may block until the decoder can decode another segment
    this->statusText = "Waiting";
    segmentIt        = this->segmentBuffer->getNextSegmentToDecode(segmentIt);
    this->statusText = "Decoding";

    this->decoder->resetDecoder();
  }
}
