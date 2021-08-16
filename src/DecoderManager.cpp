/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "DecoderManager.h"

#include "decoder/decoderVVDec.h"

#include <QDebug>
#include <chrono>

#define DEBUG_DECODER_MANAGER 1
#if DEBUG_DECODER_MANAGER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

auto START_CODE = QByteArrayLiteral("\x00\x00\x01");

}

DecoderManager::DecoderManager(ILogger *logger, FrameConversionBuffer *frameConversionBuffer)
    : logger(logger), frameConversionBuffer(frameConversionBuffer)
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

DecoderManager::~DecoderManager()
{
  this->decoderAbort = true;
  this->decoderCV.notify_one();
  this->decoderThread.join();
}

void DecoderManager::decodeFile(std::shared_ptr<File> file)
{
  assert(!this->currentFile);
  DEBUG("Starting decoding of file " << file->pathOrURL);

  std::unique_lock<std::mutex> lck(this->currentFileMutex);
  this->currentFile = file;
  this->decoderCV.notify_one();
}

bool DecoderManager::isDecodeRunning() { return bool(this->currentFile); }

void DecoderManager::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  while (true)
  {
    {
      std::unique_lock<std::mutex> lck(this->currentFileMutex);
      if (!this->currentFile)
      {
        this->decoderCV.wait(lck, [this]() { return this->currentFile || this->decoderAbort; });
      }

      if (this->decoderAbort)
        return;
      if (!this->currentFile)
        continue;
    }

    // // Simulate decoding
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1000ms);

    unsigned nrFramesDecoded = 0;
    if (auto firstPos = this->findNextNalInCurFile(0))
      this->currentDataOffset = *firstPos;
    while (true)
    {
      auto state = this->decoder->state();

      if (state == decoder::DecoderState::NeedsMoreData)
      {
        QByteArray nalData;
        if (auto nextNalStart = this->findNextNalInCurFile(this->currentDataOffset + 3))
        {
          auto length = *nextNalStart - this->currentDataOffset;
          nalData     = this->currentFile->fileData.mid(this->currentDataOffset, length);
          this->currentDataOffset = *nextNalStart;
        }

        if (!this->decoder->pushData(nalData))
        {
          this->logger->addMessage("Error pushing data", LoggingPriority::Error);
          break;
        }
      }

      if (state == decoder::DecoderState::RetrieveFrames)
      {
        if (this->decoder->decodeNextFrame())
        {
          RawYUVFrame newFrame;
          newFrame.rawData     = this->decoder->getRawFrameData();
          newFrame.pixelFormat = this->decoder->getYUVPixelFormat();
          newFrame.frameSize   = this->decoder->getFrameSize();
          this->frameConversionBuffer->addFrameToConversion(newFrame);

          DEBUG("Retrived frame " << nrFramesDecoded << " with size " << newFrame.frameSize.width
                                  << "x" << newFrame.frameSize.height);
          nrFramesDecoded++;
        }
      }

      if (state == decoder::DecoderState::Error)
      {
        this->logger->addMessage(QString("Error decoding frame %1").arg(nrFramesDecoded),
                                 LoggingPriority::Error);
                                 break;
      }

      if (state == decoder::DecoderState::EndOfBitstream)
      {
        break;
      }
    }

    if (this->decoderAbort)
      return;

    this->currentFile.reset();
    emit onDecodeOfSegmentDone();

    this->decoder->resetDecoder();
  }
}

std::optional<std::size_t> DecoderManager::findNextNalInCurFile(std::size_t start)
{
  const auto &data     = this->currentFile->fileData;
  auto        newStart = data.indexOf(START_CODE, start);
  if (newStart < 0)
    return {};
  if (newStart >= 1 && data.at(newStart - 1) == char(0x00))
    newStart -= 1;
  return newStart;
}
