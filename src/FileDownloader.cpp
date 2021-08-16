/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloader.h"

#include <QDebug>
#include <QFileInfo>

constexpr qint64 READ_SIZE = 1024 * 1024;

FileDownloader::FileDownloader(ILogger *logger) : logger(logger)
{
  this->readBuffer.resize(READ_SIZE);
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
  if (this->currentFile)
  {
    int debigagd = 2345;
    (void)debigagd;
  }
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
        this->downloaderCV.wait(lck, [this]() { return this->currentFile; });
      }

      if (this->downloaderAbort)
        return;
      if (!this->currentFile)
        continue;
    }

    if (this->downloaderAbort)
      return;

    qint64 bytesDownloaded = 0;
    if (this->currentFile->isLocalFile)
    {
      QFile inputFile(this->currentFile->pathOrURL);
      if (!inputFile.open(QIODevice::ReadOnly))
      {
        this->logger->addMessage(QString("Error reading file %1").arg(this->currentFile->pathOrURL),
                                 LoggingPriority::Error);
      }
      else if (!this->currentFile->localFile.open())
      {
        this->logger->addMessage(
            QString("Error opening temporary file %1").arg(this->currentFile->localFile.fileName()),
            LoggingPriority::Error);
      }
      else
      {
        while (true)
        {
          auto bytesRead = inputFile.read(this->readBuffer.data(), READ_SIZE);
          if (bytesRead < 0)
          {
            this->logger->addMessage(
                QString("Error while reading from file %1").arg(this->currentFile->pathOrURL),
                LoggingPriority::Error);
            break;
          }
          this->currentFile->localFile.write(this->readBuffer, bytesRead);
          if (bytesRead < READ_SIZE)
            break;

          bytesDownloaded += bytesRead;
          if (this->currentFile->nrBytes > 0)
          {
            auto newPercent = double(bytesDownloaded) / double(this->currentFile->nrBytes);
            this->currentFile->downloadProgress.store(newPercent);
          }

          if (this->downloaderAbort)
            return;
        }
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
