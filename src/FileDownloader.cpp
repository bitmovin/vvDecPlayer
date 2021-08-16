/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloader.h"

#include <QDebug>
#include <QFileInfo>

constexpr qint64 READ_SIZE = 1024 * 1024;

FileDownloader::FileDownloader(ILogger *logger) : logger(logger)
{
  this->downloaderThread = std::thread(&FileDownloader::runDownloader, this);
  this->readBuffer.resize(READ_SIZE);
}

void FileDownloader::downloadLocalFile(QString pathOrURL)
{
  assert(!this->currentFile);

  QFileInfo fileInfo(pathOrURL);

  auto f         = std::make_shared<File>();
  f->pathOrURL   = pathOrURL;
  f->isLocalFile = true;
  f->nrBytes     = std::size_t(fileInfo.size());

  std::scoped_lock lock(this->currentFileMutex);
  this->currentFile = f;
  this->downloaderCV.notify_one();
}

std::shared_ptr<FileDownloader::File> FileDownloader::getDownloadedFile()
{
  if (this->downloadQueueDone.empty() || this->downloadQueueDone.front()->downloadProgress != 100.0)
    return {};

  auto file = this->downloadQueueDone.front();
  this->downloadQueueDone.pop();
  return file;
}

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
    if (currentFile->isLocalFile)
    {
      QFile inputFile(currentFile->pathOrURL);
      if (!inputFile.open(QIODevice::ReadOnly))
      {
        this->logger->addMessage(QString("Error reading file %1").arg(currentFile->pathOrURL),
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
                QString("Error while reading from file %1").arg(currentFile->pathOrURL),
                LoggingPriority::Error);
            break;
          }
          currentFile->localFile.write(this->readBuffer, bytesRead);
          if (bytesRead < READ_SIZE)
            break;

          bytesDownloaded += bytesRead;
          if (currentFile->nrBytes > 0)
          {
            auto newPercent = double(bytesDownloaded) / double(currentFile->nrBytes);
            currentFile->downloadProgress.store(newPercent);
          }

          if (this->downloaderAbort)
            return;
        }
      }
    }

    currentFile->downloadProgress.store(100.0);
    emit downloadDone();

    std::unique_lock<std::mutex> lck(this->downloadQueueDoneMutex);
    this->downloadQueueDone.push(currentFile);
    currentFile.reset();
  }
}
