/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FrameConversionThread.h"
#include <YUV/YUVConversion.h>

#define DEBUG_CONVERSION 0
#if DEBUG_CONVERSION
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

FrameConversionThread::FrameConversionThread(ILogger *logger, SegmentBuffer *segmentBuffer)
    : logger(logger), segmentBuffer(segmentBuffer)
{
  this->conversionThread = std::thread(&FrameConversionThread::runConversion, this);
}

FrameConversionThread::~FrameConversionThread() { this->abort(); }

void FrameConversionThread::abort()
{
  this->conversionAbort = true;
  if (this->conversionThread.joinable())
    this->conversionThread.join();
}

QString FrameConversionThread::getStatus()
{
  return (this->conversionAbort ? "Abort " : "") + this->statusText;
}

void FrameConversionThread::runConversion()
{
  uint64_t frameCounter{};
  auto     frameIt = this->segmentBuffer->getFirstFrameToConvert();
  while (true)
  {
    if (this->conversionAbort)
      return;

    DEBUG("Conversion Thread: Convert Frame " << frameCounter);
    convertYUVToImage(
        frameIt.frame->rawYUVData, frameIt.frame->rgbImage, frameIt.frame->pixelFormat, frameIt.frame->frameSize);
    frameIt.frame->frameState = FrameState::ConvertedToRGB;
    this->conversionRunning.store(false);
    DEBUG("Conversion Thread: Frame " << frameCounter << " done.");
    frameCounter++;

    // This may block until a frame is available
    this->conversionRunning.store(false);
    this->statusText = "Paused";
    frameIt          = this->segmentBuffer->getNextFrameToConvert(frameIt);
    this->conversionRunning.store(true);
    this->statusText = "Running";
  }

  statusText = "Thread stopped";
}
