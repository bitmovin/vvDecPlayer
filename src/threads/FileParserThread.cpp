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

#include "FileParserThread.h"

#include <common/functions.h>
#include <parser/VVC/AnnexBVVC.h>

#include <QDebug>
#include <chrono>

#define DEBUG_PARSER 0
#if DEBUG_PARSER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

FileParserThread::FileParserThread(ILogger *logger, SegmentBuffer *segmentBuffer)
    : logger(logger), segmentBuffer(segmentBuffer)
{
  this->parserThread = std::thread(&FileParserThread::runParser, this);
}

FileParserThread::~FileParserThread()
{
  this->abort();
  if (this->parserThread.joinable())
    this->parserThread.join();
}

void FileParserThread::abort() { this->parserAbort = true; }

QString FileParserThread::getStatus() const
{
  return (this->parserAbort ? "Abort " : "") + this->statusText;
}

void FileParserThread::runParser()
{
  this->logger->addMessage("Started parser thread", LoggingPriority::Info);

  size_t currentDataOffset{};
  auto   segmentIt = this->segmentBuffer->getFirstSegmentToParse();

  while (!this->parserAbort)
  {
    const auto &data = segmentIt->compressedData;

    parser::AnnexBVVC parser;

    int nalID = 0;
    if (auto firstPos = findNextNalInData(data, 0))
      currentDataOffset = *firstPos;
    bool isLastNalInData = false;
    while (!this->parserAbort)
    {
      QByteArray nalData;

      if (!isLastNalInData)
      {
        if (auto nextNalStart = findNextNalInData(data, currentDataOffset + 3))
        {
          auto length       = *nextNalStart - currentDataOffset;
          nalData           = data.mid(currentDataOffset, length);
          currentDataOffset = *nextNalStart;
        }
        else if (currentDataOffset < size_t(data.size()))
        {
          nalData           = data.mid(currentDataOffset);
          currentDataOffset = data.size();
          isLastNalInData   = true;
        }
      }
      else
        nalID = -1;

      DEBUG("Parsing NAL of " << nalData.size() << " bytes");
      auto parseResult = parser.parseAndAddNALUnit(nalID, convertToByteVector(nalData), {});
      if (parseResult.success)
      {
        if (parseResult.bitrateEntry)
        {
          // New AU
          DEBUG("AU PTS:" << parseResult.bitrateEntry->pts
                          << " bitrate:" << parseResult.bitrateEntry->bitrate);
          auto newFrame = segmentBuffer->getNewFrame();
          newFrame->nrBytesCompressed = parseResult.bitrateEntry->bitrate;
          newFrame->poc = parseResult.bitrateEntry->pts;
          segmentIt->frames.push_back(newFrame);
        }
      }
      else
      {
        this->logger->addMessage(
            QString("Error parsing nal %1 in Segment %2").arg(nalID).arg(segmentIt->segmentNumber),
            LoggingPriority::Error);
      }

      if (nalID == -1)
        break;

      nalID++;
    }

    segmentIt->parsingFinished = true;

    if (this->parserAbort)
      return;

    // This may block until there is another segment to parse
    this->statusText = "Waiting";
    segmentIt        = this->segmentBuffer->getNextSegmentToParse(segmentIt);
    this->statusText = "Parsing";
  }
}
