/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <SegmentBuffer.h>
#include <condition_variable>
#include <decoder/decoderBase.h>
#include <optional>
#include <thread>

class DecoderThread
{
public:
  DecoderThread(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~DecoderThread();
  void abort();

  QString getStatus();

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  void runDecoder();

  std::unique_ptr<decoder::decoderBase> decoder;

  std::thread decoderThread;
  bool        decoderAbort{false};

  QString statusText;
};
