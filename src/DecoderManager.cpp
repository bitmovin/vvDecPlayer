/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "DecoderManager.h"

#include "decoder/decoderVVDec.h"

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

auto START_CODE = QByteArrayLiteral("\x00\x00\x01");

std::optional<std::size_t> findNextNalInCurFile(QByteArray &data, std::size_t start)
{
  auto newStart = data.indexOf(START_CODE, start);
  if (newStart < 0)
    return {};
  if (newStart >= 1 && data.at(newStart - 1) == char(0x00))
    newStart -= 1;
  return newStart;
}

} // namespace

DecoderManager::DecoderManager(ILogger *logger, SegmentBuffer *segmentBuffer)
    : logger(logger), segmentBuffer(segmentBuffer)
{
  this->decoder = std::make_unique<decoder::decoderVVDec>();
  if (this->decoder->errorInDecoder())
  {
    this->logger->addMessage("Error in decoder: " + this->decoder->decoderErrorString(),
                             LoggingPriority::Error);
    return;
  }

  this->decoderThread = std::thread(&DecoderManager::runDecoder, this);
}

DecoderManager::~DecoderManager() { this->abort(); }

void DecoderManager::abort()
{
  this->decoderAbort = true;
  if (this->decoderThread.joinable())
  {
    this->decoderThread.join();
  }
}

QString DecoderManager::getStatus()
{
  return (this->decoderAbort ? "Abort " : "") + this->statusText;
}

void DecoderManager::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  size_t currentDataOffset{};
  auto   segmentIt = this->segmentBuffer->getFirstSegmentToDecode();

  while (true)
  {
    // // Simulate decoding
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1000ms);

    this->currentFrameIdxInSegment = 0;
    if (auto firstPos = findNextNalInCurFile(segmentIt->compressedData, 0))
      currentDataOffset = *firstPos;
    while (true)
    {
      auto state = this->decoder->state();

      if (state == decoder::DecoderState::NeedsMoreData)
      {
        QByteArray nalData;
        if (auto nextNalStart =
                findNextNalInCurFile(segmentIt->compressedData, currentDataOffset + 3))
        {
          auto length       = *nextNalStart - currentDataOffset;
          nalData           = segmentIt->compressedData.mid(currentDataOffset, length);
          currentDataOffset = *nextNalStart;
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
          Frame newFrame;
          newFrame.rawYUVData  = this->decoder->getRawFrameData();
          newFrame.pixelFormat = this->decoder->getYUVPixelFormat();
          newFrame.frameSize   = this->decoder->getFrameSize();
          segmentIt->frames.push_back(newFrame);

          DEBUG("Retrived frame " << this->currentFrameIdxInSegment << " with size "
                                  << newFrame.frameSize.width << "x" << newFrame.frameSize.height);
          this->currentFrameIdxInSegment++;
        }
      }

      if (state == decoder::DecoderState::Error)
      {
        DEBUG("Decoding error");
        this->logger->addMessage(
            QString("Error decoding frame %1").arg(this->currentFrameIdxInSegment),
            LoggingPriority::Error);
        break;
      }

      if (state == decoder::DecoderState::EndOfBitstream)
      {
        DEBUG("Decoding EOF");
        break;
      }

      if (this->decoderAbort)
        return;
    }

    if (this->decoderAbort)
      return;

    // This may block until the decoder can decode another segment
    this->statusText = "Waiting";
    segmentIt        = this->segmentBuffer->getNextSegmentToDecode(segmentIt);
    this->statusText = "Decoding";

    if (this->decoderAbort)
      return;

    this->decoder->resetDecoder();
  }
}
