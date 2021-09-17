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

FrameConversionThread::~FrameConversionThread()
{
  this->abort();
  if (this->conversionThread.joinable())
    this->conversionThread.join();
}

void FrameConversionThread::abort() { this->conversionAbort = true; }

QString FrameConversionThread::getStatus() const
{
  return (this->conversionAbort ? "Abort " : "") + this->statusText;
}

void FrameConversionThread::runConversion()
{
  uint64_t frameCounter{};
  auto     frameIt = this->segmentBuffer->getFirstFrameToConvert();
  while (!this->conversionAbort)
  {
    DEBUG("Conversion Thread: Convert Frame " << frameCounter);
    convertYUVToImage(frameIt.frame->rawYUVData,
                      frameIt.frame->rgbImage,
                      frameIt.frame->pixelFormat,
                      frameIt.frame->frameSize);
    frameIt.frame->frameState = FrameState::ConvertedToRGB;
    this->conversionRunning.store(false);
    DEBUG("Conversion Thread: Frame " << frameCounter << " done.");
    frameCounter++;

    if (this->conversionAbort)
      break;

    // This may block until a frame is available
    this->conversionRunning.store(false);
    this->statusText = "Paused";
    frameIt          = this->segmentBuffer->getNextFrameToConvert(frameIt);
    this->conversionRunning.store(true);
    this->statusText = "Running";
  }

  statusText = "Thread stopped";
}
