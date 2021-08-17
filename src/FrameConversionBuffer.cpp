/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FrameConversionBuffer.h"
#include <YUV/YUVConversion.h>

#define DEBUG_CONVERSION 0
#if DEBUG_CONVERSION
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

constexpr auto MAX_QUEUE_SIZE = 24;

}

FrameConversionBuffer::FrameConversionBuffer()
{
  this->conversionThread = std::thread(&FrameConversionBuffer::runConversion, this);
}

FrameConversionBuffer::~FrameConversionBuffer() { this->abort(); }

void FrameConversionBuffer::abort()
{
  this->conversionAbort = true;
  if (this->conversionThread.joinable())
  {
    this->conversionCV.notify_one();
    this->bufferFullCV.notify_one();
    this->conversionThread.join();
  }
}

void FrameConversionBuffer::addFrameToConversion(RawYUVFrame frame)
{
  DEBUG("Conversion: Adding frame to queue. Queue size " << this->framesToConvert.size());
  std::unique_lock<std::mutex> lck(this->framesToConvertMutex);
  if (!(this->framesToConvert.size() < MAX_QUEUE_SIZE || this->conversionAbort))
  {
    DEBUG("Conversion: Waiting for space in queue");
    this->bufferFullCV.wait(lck, [this]() {
      return this->framesToConvert.size() < MAX_QUEUE_SIZE || this->conversionAbort;
    });
    DEBUG("Conversion: Space in queue. Pusing frame.");
  }

  this->framesToConvert.push(frame);
  this->conversionCV.notify_one();
}

std::optional<QImage> FrameConversionBuffer::getNextImage()
{
  std::unique_lock<std::mutex> lck(this->convertedFramesMutex);
  if (this->convertedFrames.empty())
    return {};
  auto nextFrame = this->convertedFrames.front();
  this->convertedFrames.pop();
  this->conversionCV.notify_one();
  return nextFrame;
}

QString FrameConversionBuffer::getStatus()
{
  if (this->conversionAbort)
    return "Conversion: Aborted";
  return QString("Conversion: YUV buffer %1 RGB buffer %2 %3")
      .arg(this->framesToConvert.size())
      .arg(this->convertedFrames.size())
      .arg(this->conversionRunning.load() ? "Run" : "Pause");
}

void FrameConversionBuffer::addFrameQueueInfo(std::vector<FrameStatus> &info)
{
  for (size_t i = 0; i < this->convertedFrames.size(); i++)
    info.push_back(FrameStatus(FrameState::Converted));
  if (this->conversionRunning.load())
    info.push_back(FrameStatus(FrameState::Decoded, true));
  for (size_t i = 0; i < this->framesToConvert.size(); i++)
    info.push_back(FrameStatus(FrameState::Decoded));
}

void FrameConversionBuffer::runConversion()
{
  uint64_t frameCounter{};
  while (true)
  {
    RawYUVFrame currentFrame;
    {
      std::unique_lock<std::mutex> lck(this->framesToConvertMutex);
      this->conversionCV.wait(lck, [this]() {
        return (!this->framesToConvert.empty() && this->convertedFrames.size() < MAX_QUEUE_SIZE) ||
               this->conversionAbort;
      });
      this->conversionRunning.store(true);

      if (this->conversionAbort)
        return;

      currentFrame = this->framesToConvert.front();
      this->framesToConvert.pop();
      this->bufferFullCV.notify_one();
    }

    DEBUG("Conversion Thread: Convert Frame " << frameCounter);
    QImage outputImage;
    convertYUVToImage(
        currentFrame.rawData, outputImage, currentFrame.pixelFormat, currentFrame.frameSize);
    DEBUG("Conversion Thread: Push output");
    std::unique_lock<std::mutex> lck(this->convertedFramesMutex);
    this->conversionRunning.store(false);
    this->convertedFrames.push(outputImage);
    DEBUG("Conversion Thread: Frame " << frameCounter << " done.");
    frameCounter++;
  }
}
