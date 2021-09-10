/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <SegmentBuffer.h>
#include <condition_variable>
#include <decoder/decoderBase.h>
#include <optional>
#include <thread>

class FileParserThread
{
public:
  FileParserThread(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~FileParserThread();
  void abort();

  QString getStatus() const;

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  void runParser();

  std::unique_ptr<decoder::decoderBase> decoder;

  std::thread parserThread;
  bool        parserAbort{false};

  QString statusText;
};
