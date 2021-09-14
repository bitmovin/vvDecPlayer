/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <SegmentBuffer.h>
#include <common/Frame.h>
#include <common/ILogger.h>

#include <QByteArray>
#include <QImage>
#include <condition_variable>
#include <optional>
#include <queue>
#include <thread>

class FrameConversionThread
{
public:
  FrameConversionThread(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~FrameConversionThread();
  void abort();

  QString getStatus() const;

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  void runConversion();

  std::thread conversionThread;
  bool        conversionAbort{false};

  std::condition_variable bufferFullCV;

  std::atomic_bool conversionRunning{false};

  QString statusText;
};
