/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QByteArray>
#include <QImage>
#include <common/RawFrame.h>
#include <condition_variable>
#include <optional>
#include <queue>
#include <thread>

class FrameConversionBuffer
{
public:
  FrameConversionBuffer();
  ~FrameConversionBuffer();

  // May block if the queue is too full
  void addFrameToConversion(RawYUVFrame frame);

  std::optional<QImage> getNextImage();

private:
  void runConversion();

  std::thread             conversionThread;
  std::condition_variable conversionCV;
  bool                    conversionAbort{false};

  std::condition_variable bufferFullCV;

  std::queue<RawYUVFrame> framesToConvert;
  std::queue<QImage>      convertedFrames;
  std::mutex              framesToConvertMutex;
  std::mutex              convertedFramesMutex;
};
