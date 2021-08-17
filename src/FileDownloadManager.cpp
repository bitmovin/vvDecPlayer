/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloadManager.h"

namespace
{

constexpr unsigned MAX_FILES_IN_QUEUE = 5;

}

FileDownloadManager::FileDownloadManager(ILogger *logger) : logger(logger)
{
  assert(logger != nullptr);
  this->fileDownloader = std::make_unique<FileDownloader>(this->logger);
  connect(this->fileDownloader.get(),
          &FileDownloader::downloadDone,
          this,
          &FileDownloadManager::onDownloadDone);
}

void FileDownloadManager::abort() { this->fileDownloader->abort(); }

void FileDownloadManager::openDirectory(QDir path, QString segmentPattern)
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

  if (segmentNr > 0)
  {
    this->currentFileIt = this->localFileList.begin();
    this->fileDownloader->downloadLocalFile(*this->currentFileIt);
  }
}

std::shared_ptr<File> FileDownloadManager::getNextDownloadedFile()
{
  auto file = this->fileDownloader->getNextDownloadedFile();
  if (!this->fileDownloader->isDownloadRunning() &&
      this->fileDownloader->nrFilesInDownloadedQueue() < MAX_FILES_IN_QUEUE)
    this->startDownloadOfNextFile();
  return file;
}

QString FileDownloadManager::getStatus()
{
  if (this->fileDownloader)
    return this->fileDownloader->getStatus();
  return {};
}

void FileDownloadManager::addFrameQueueInfo(std::vector<FrameStatus> &info)
{
  this->fileDownloader->addFrameQueueInfo(info);
}

auto FileDownloadManager::getLastSegmentsData() -> std::deque<SegmentData>
{
  return this->fileDownloader->getLastSegmentsData();
}

void FileDownloadManager::setSegmentLength(unsigned segmentLength)
{
  this->fileDownloader->setSegmentLength(segmentLength);
}

void FileDownloadManager::onDownloadDone()
{
  emit onSegmentReadyForDecode();

  if (!this->fileDownloader->isDownloadRunning() &&
      this->fileDownloader->nrFilesInDownloadedQueue() < MAX_FILES_IN_QUEUE)
    this->startDownloadOfNextFile();
}

void FileDownloadManager::startDownloadOfNextFile()
{
  this->currentFileIt++;
  if (this->currentFileIt == this->localFileList.end())
    this->currentFileIt = this->localFileList.begin();
  this->fileDownloader->downloadLocalFile(*this->currentFileIt);
}
