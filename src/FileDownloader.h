/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <common/Segment.h>
#include <common/typedef.h>
#include <SegmentBuffer.h>

#include <QDir>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

/* A background downloader for files.
 *
 * In a background thread it will always try to keep a certain number of files in a local
 * cache. When a download is done, we will provide a report on the download (e.g. start, end,
 * nrBytes). The DownloadFilePlanner can then take that data and add the next file to download to
 * the list.
 */
class FileDownloader
{
public:
  FileDownloader(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~FileDownloader();
  void abort();

  void openDirectory(QDir path, QString segmentPattern);

  QString getStatus();

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  void        runDownloader();
  std::thread downloaderThread;
  bool        downloaderAbort{false};

  std::vector<QString> localFileList;

  bool isLocalSource{false};

  QString statusText;

  unsigned segmentLength{24};
};
