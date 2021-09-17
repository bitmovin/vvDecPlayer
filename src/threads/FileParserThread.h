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

#pragma once

#include <SegmentBuffer.h>
#include <common/ILogger.h>
#include <decoder/decoderBase.h>

#include <condition_variable>
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
