/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

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

QString FileParserThread::getStatus()
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

    unsigned nalID = 0;
    if (auto firstPos = findNextNalInData(data, 0))
      currentDataOffset = *firstPos;
    bool isLastNalInData = false;
    while (!this->parserAbort && !isLastNalInData)
    {
      QByteArray nalData;

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

      DEBUG("Parsing NAL of " << nalData.size() << " bytes");
      parser.parseAndAddNALUnit(nalID, convertToByteVector(nalData), {});
      nalID++;
    }

    if (this->parserAbort)
      return;

    // This may block until there is another segment to parse
    this->statusText = "Waiting";
    segmentIt        = this->segmentBuffer->getNextSegmentToParse(segmentIt);
    this->statusText = "Decoding";
  }
}
