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

DecoderManager::~DecoderManager() { this->abort(); }

void DecoderManager::abort()
{
  this->decoderAbort = true;
  if (this->decoderThread.joinable())
  {
    this->decoderCV.notify_one();
    this->decoderThread.join();
  }
}

void DecoderManager::decodeFile(std::shared_ptr<File> file)
{
  assert(!this->currentFile);
  DEBUG("Starting decoding of file " << file->pathOrURL);

  std::unique_lock<std::mutex> lck(this->currentFileMutex);
  this->currentFile = file;
  this->decoderCV.notify_one();
}

QString DecoderManager::getStatus()
{
  if (this->decoderState == DecoderState::Running)
    return QString("Decoder: Decoding file. Frame %1").arg(this->currentDecodeFrame);
  else if (this->decoderState == DecoderState::WaitingForNextFile)
    return "Decoder: Waiting for next file to decode.";
  else if (this->decoderState == DecoderState::WaitingToPushOutput)
    return QString("Decoder: Decoding file. Frame %1 - pushing out").arg(this->currentDecodeFrame);
  return {"Decoder: "};
}

bool DecoderManager::isDecodeRunning() { return bool(this->currentFile); }

void DecoderManager::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  while (true)
  {
    {
      std::unique_lock<std::mutex> lck(this->currentFileMutex);
      if (!this->currentFile && !this->decoderAbort)
      {
        DEBUG("Decoder Waiting for next file");
        this->decoderState = DecoderState::WaitingForNextFile;
        this->decoderCV.wait(lck, [this]() { return this->currentFile || this->decoderAbort; });
        this->decoderState = DecoderState::Running;
      }

      if (this->decoderAbort)
        return;
      if (!this->currentFile)
        continue;
    }

    // // Simulate decoding
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1000ms);

    this->currentDecodeFrame = 0;
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
          RawYUVFrame newFrame;
          newFrame.rawData     = this->decoder->getRawFrameData();
          newFrame.pixelFormat = this->decoder->getYUVPixelFormat();
          newFrame.frameSize   = this->decoder->getFrameSize();
          
          this->decoderState = DecoderState::WaitingToPushOutput;
          this->frameConversionBuffer->addFrameToConversion(newFrame);
          this->decoderState = DecoderState::Running;

          DEBUG("Retrived frame " << this->currentDecodeFrame << " with size "
                                  << newFrame.frameSize.width << "x" << newFrame.frameSize.height);
          this->currentDecodeFrame++;
        }
      }

      if (state == decoder::DecoderState::Error)
      {
        DEBUG("Decoding error");
        this->logger->addMessage(QString("Error decoding frame %1").arg(this->currentDecodeFrame),
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

    this->currentFile.reset();
    emit onDecodeOfSegmentDone();
    DEBUG("Decoding of segment done");

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
