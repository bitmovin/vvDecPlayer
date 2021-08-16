/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "DecoderManager.h"

#include "decoder/decoderVVDec.h"

#include <chrono>
#include <QDebug>

DecoderManager::DecoderManager(ILogger *logger) : logger(logger)
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
  qDebug() << "Starting decoding of file " << file->pathOrURL;

  std::unique_lock<std::mutex> lck(this->currentFileMutex);
  this->currentFile = file;
  this->decoderCV.notify_one();
}

bool DecoderManager::isDecodeRunning()
{
  return bool(this->currentFile);
}

void DecoderManager::runDecoder()
{
  this->logger->addMessage("Started decoder thread", LoggingPriority::Info);

  while (true)
  {
    {
      std::unique_lock<std::mutex> lck(this->currentFileMutex);
      if (!this->currentFile)
      {
        this->decoderCV.wait(lck, [this]() { return this->currentFile; });
      }

      if (this->decoderAbort)
        return;
      if (!this->currentFile)
        continue;
    }

    // Simulate decoding
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1000ms);

    if (this->decoderAbort)
      return;

    this->currentFile.reset();
    emit onDecodeOfSegmentDone();
  }
}

