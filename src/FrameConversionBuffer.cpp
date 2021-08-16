/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FrameConversionBuffer.h"
#include <YUV/YUVConversion.h>

#define DEBUG_CONVERSION 1
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

FrameConversionBuffer::~FrameConversionBuffer()
{
  this->conversionAbort = true;
  this->conversionCV.notify_one();
  this->bufferFullCV.notify_one();
  this->conversionThread.join();
}

void FrameConversionBuffer::addFrameToConversion(RawYUVFrame frame)
{
  DEBUG("Adding frame to queue. Queue size " << this->framesToConvert.size());

  std::unique_lock<std::mutex> lck(this->framesToConvertMutex);
  this->bufferFullCV.wait(lck, [this]() {
    return this->framesToConvert.size() < MAX_QUEUE_SIZE || this->conversionAbort;
  });

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
  return nextFrame;
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

      if (this->conversionAbort)
        return;

      currentFrame = this->framesToConvert.front();
      this->framesToConvert.pop();
      this->bufferFullCV.notify_one();
    }

    DEBUG("Converting frame " << frameCounter);
    QImage outputImage;
    convertYUVToImage(
        currentFrame.rawData, outputImage, currentFrame.pixelFormat, currentFrame.frameSize);
    std::unique_lock<std::mutex> lck(this->convertedFramesMutex);
    this->convertedFrames.push(outputImage);
    frameCounter++;
  }
}
