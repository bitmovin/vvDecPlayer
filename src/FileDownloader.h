/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"

#include <QByteArray>
#include <QObject>
#include <QTemporaryFile>
#include <atomic>
#include <chrono>
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

  void downloadLocalFile(QString pathOrURL);

  using Time = std::chrono::time_point<std::chrono::steady_clock>;
  struct File
  {
    QString        pathOrURL;
    QTemporaryFile localFile;
    Time           start{};
    Time           end{};
    std::size_t    nrBytes{};
    bool           isLocalFile{};

    using Percent = double;
    std::atomic<Percent> downloadProgress{0.0};
  };

  std::shared_ptr<File> getDownloadedFile();

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

  QByteArray readBuffer;
};
