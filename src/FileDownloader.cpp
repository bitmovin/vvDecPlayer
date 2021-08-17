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

FileDownloader::FileDownloader(ILogger *logger) : logger(logger)
{
  this->downloaderThread = std::thread(&FileDownloader::runDownloader, this);
}

FileDownloader::~FileDownloader() { this->abort(); }

void FileDownloader::abort()
{
  this->downloaderAbort = true;
  if (this->downloaderThread.joinable())
  {
    this->downloaderCV.notify_one();
    this->downloaderThread.join();
  }
}

void FileDownloader::downloadLocalFile(QString pathOrURL)
{
  assert(!this->currentFile);

  QFileInfo fileInfo(pathOrURL);

  auto f         = std::make_shared<File>();
  f->pathOrURL   = pathOrURL;
  f->isLocalFile = true;
  f->nrBytes     = std::size_t(fileInfo.size());

  DEBUG("Starting download of file " << pathOrURL);

  std::scoped_lock lock(this->currentFileMutex);
  this->currentFile = f;
  this->downloaderCV.notify_one();
}

std::shared_ptr<File> FileDownloader::getNextDownloadedFile()
{
  if (this->downloadQueueDone.empty() || this->downloadQueueDone.front()->downloadProgress != 100.0)
    return {};

  auto file = this->downloadQueueDone.front();
  this->downloadQueueDone.pop();
  return file;
}

std::size_t FileDownloader::nrFilesInDownloadedQueue() { return this->downloadQueueDone.size(); }

bool FileDownloader::isDownloadRunning() { return bool(this->currentFile); }

QString FileDownloader::getStatus()
{
  std::unique_lock<std::mutex> lckFile(this->currentFileMutex);
  QString status = "Downloader: ";
  if (this->currentFile)
  {
    auto percent = int(this->currentFile->downloadProgress.load());
    status += QString("Downloading progress %d%%").arg(percent);
  }
  else
    status += "Waiting for next file";
  status += QString(" - out %1").arg(this->downloadQueueDone.size());
  return status;
}

void FileDownloader::runDownloader()
{
  this->logger->addMessage("Started downloader thread", LoggingPriority::Info);

  while (true)
  {
    {
      std::unique_lock<std::mutex> lck(this->currentFileMutex);
      this->downloaderCV.wait(lck, [this]() { return this->currentFile || this->downloaderAbort; });

      if (this->downloaderAbort)
        return;
      if (!this->currentFile)
        continue;
    }

    if (this->currentFile->isLocalFile)
    {
      QFile inputFile(this->currentFile->pathOrURL);
      if (!inputFile.open(QIODevice::ReadOnly))
      {
        this->logger->addMessage(QString("Error reading file %1").arg(this->currentFile->pathOrURL),
                                 LoggingPriority::Error);
      }
      else
      {
        this->currentFile->fileData = inputFile.readAll();
      }
    }

    {
      std::unique_lock<std::mutex> lckQueue(this->downloadQueueDoneMutex);
      std::unique_lock<std::mutex> lckFile(this->currentFileMutex);
      this->currentFile->downloadProgress.store(100.0);
      this->downloadQueueDone.push(this->currentFile);
      this->currentFile.reset();
    }

    DEBUG("Download done");
    emit downloadDone();
  }
}
