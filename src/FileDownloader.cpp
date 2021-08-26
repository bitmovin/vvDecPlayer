/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloader.h"

#include <QDebug>
#include <QFileInfo>

#define DEBUG_DOWNLOADER 0
#if DEBUG_DOWNLOADER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

FileDownloader::FileDownloader(ILogger *logger, SegmentBuffer *segmentBuffer)
    : logger(logger), segmentBuffer(segmentBuffer)
{
}

FileDownloader::~FileDownloader()
{
  this->abort();
  if (this->downloaderThread.joinable())
    this->downloaderThread.join();
}

void FileDownloader::abort() { this->downloaderAbort = true; }

QString FileDownloader::getStatus()
{
  return (this->downloaderAbort ? "Abort " : "") + this->statusText;
}

void FileDownloader::openDirectory(QDir path, QString segmentPattern)
{
  unsigned segmentNr = 0;
  while (true)
  {
    auto file = segmentPattern;
    file.replace("%i", QString("%1").arg(segmentNr));
    if (!path.exists(file))
      break;

    auto fullFilePath = path.filePath(file);
    this->localFileList.push_back(fullFilePath);

    segmentNr++;
  }

  this->logger->addMessage(QString("Found %1 local files to play.").arg(segmentNr),
                           LoggingPriority::Info);

  this->isLocalSource    = true;
  this->downloaderThread = std::thread(&FileDownloader::runDownloader, this);
}

void FileDownloader::runDownloader()
{
  this->logger->addMessage("Started downloader thread", LoggingPriority::Info);

  auto fileIt = this->localFileList.begin();

  while (!this->downloaderAbort)
  {
    if (this->isLocalSource)
    {
      QFile inputFile(*fileIt);
      if (!inputFile.open(QIODevice::ReadOnly))
      {
        this->logger->addMessage(QString("Error reading file %1").arg(*fileIt),
                                 LoggingPriority::Error);
      }
      else
      {
        auto newSegment            = std::make_shared<Segment>();
        newSegment->compressedData = inputFile.readAll();

        this->statusText = "Waiting";
        this->segmentBuffer->pushDownloadedSegment(newSegment);
        this->statusText = "Running";
      }
    }
    else
    {
      assert(false);
    }

    fileIt++;
  }

  this->statusText = "Finished";
}
