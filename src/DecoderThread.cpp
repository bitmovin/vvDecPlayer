/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "DecoderThread.h"

#include <decoder/decoderVVDec.h>
#include <common/functions.h>

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

QString DecoderThread::getStatus()
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
          auto newFrame         = std::make_shared<Frame>();
          newFrame->rawYUVData  = this->decoder->getRawFrameData();
          newFrame->pixelFormat = this->decoder->getYUVPixelFormat();
          newFrame->frameSize   = this->decoder->getFrameSize();
          newFrame->frameState  = FrameState::Decoded;
          segmentIt->frames.push_back(newFrame);

          DEBUG("Retrived frame " << currentFrameIdxInSegment << " with size "
                                  << newFrame->frameSize.width << "x"
                                  << newFrame->frameSize.height);
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
