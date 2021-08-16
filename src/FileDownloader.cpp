/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloader.h"

#include <QDebug>
#include <QFileInfo>

FileDownloader::FileDownloader(ILogger *logger) : logger(logger)
{
  this->downloaderThread = std::thread(&FileDownloader::runDownloader, this);
}

FileDownloader::~FileDownloader()
{
  this->downloaderAbort = true;
  this->downloaderCV.notify_one();
  this->downloaderThread.join();
}

void FileDownloader::downloadLocalFile(QString pathOrURL)
{
  assert(!this->currentFile);

  QFileInfo fileInfo(pathOrURL);

  auto f         = std::make_shared<File>();
  f->pathOrURL   = pathOrURL;
  f->isLocalFile = true;
  f->nrBytes     = std::size_t(fileInfo.size());

  qDebug() << "Starting download of file " << pathOrURL;

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

void FileDownloader::runDownloader()
{
  this->logger->addMessage("Started downloader thread", LoggingPriority::Info);

  while (true)
  {
    {
      std::unique_lock<std::mutex> lck(this->currentFileMutex);
      if (!this->currentFile)
      {
        this->downloaderCV.wait(lck, [this]() { return this->currentFile || this->downloaderAbort; });
      }

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

    emit downloadDone();
  }
}
