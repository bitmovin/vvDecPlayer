/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <common/File.h>

#include <QByteArray>
#include <QObject>
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
class FileDownloader : public QObject
{
  Q_OBJECT

public:
  FileDownloader(ILogger *logger);
  ~FileDownloader();

  void downloadLocalFile(QString pathOrURL);

  std::shared_ptr<File> getNextDownloadedFile();
  std::size_t           nrFilesInDownloadedQueue();
  bool                  isDownloadRunning();

signals:

  void downloadDone();

private:
  ILogger *logger{};

  std::shared_ptr<File> currentFile;
  std::mutex            currentFileMutex;

  std::queue<std::shared_ptr<File>> downloadQueueDone;
  std::mutex                        downloadQueueDoneMutex;

  void                    runDownloader();
  std::thread             downloaderThread;
  std::condition_variable downloaderCV;
  bool                    downloaderAbort{false};
};
