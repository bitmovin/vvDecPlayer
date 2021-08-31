/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "FileDownloader.h"

#include <QDebug>
#include <QFileInfo>
#include <QNetworkReply>

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
  DEBUG("FileDownloader - Built with SSL version: " << QSslSocket::sslLibraryBuildVersionString());
  DEBUG("FileDownloader - Found SSL library: " << QSslSocket::sslLibraryVersionString());
  DEBUG("FileDownloader - SSL supported: " << QSslSocket::supportsSsl());

  connect(&this->networkManager,
          &QNetworkAccessManager::finished,
          this,
          &FileDownloader::replyFinished);

  connect(
      segmentBuffer, &SegmentBuffer::startNextDownload, this, &FileDownloader::downloadNextFile);
  connect(
      this, &FileDownloader::downloadFinished, segmentBuffer, &SegmentBuffer::onDownloadFinished);
}

QString FileDownloader::getStatus() { return this->statusText; }

void FileDownloader::replyFinished(QNetworkReply *reply)
{
  DEBUG("Reply finished");
  if (reply->error() != QNetworkReply::NoError)
  {
    DEBUG("Error " << reply->errorString());
    this->logger->addMessage(QString("Download Error: %s").arg(reply->errorString()),
                             LoggingPriority::Error);
    return;
  }

  this->currentSegment->compressedData      = reply->readAll();
  this->currentSegment->downloadProgress    = 100.0;
  this->currentSegment->downloadFinished    = true;
  this->currentSegment->compressedSizeBytes = this->currentSegment->compressedData.size();

  emit downloadFinished();

  this->statusText = "Waiting";
}

void FileDownloader::updateDownloadProgress(int64_t val, int64_t max)
{
  DEBUG("Download Progress V " << val << " MAX " << max);
  if (max > 0 && val > 0)
  {
    auto downloadPercent = val * 100 / max;
    if (this->currentSegment)
    {
      this->currentSegment->downloadProgress    = Segment::Percent(downloadPercent);
      this->currentSegment->compressedSizeBytes = size_t(max);
    }
  }
}

void FileDownloader::openDirectory(QDir path, QString segmentPattern)
{
  DEBUG("Open URL " << path << " with pattern " << segmentPattern);
  unsigned segmentNr = 0;
  while (true)
  {
    auto file = segmentPattern;
    file.replace("%i", QString("%1").arg(segmentNr));
    if (!path.exists(file))
      break;

    auto fullFilePath = path.filePath(file);
    this->fileList.push_back(fullFilePath);

    segmentNr++;
  }

  this->logger->addMessage(QString("Found %1 local files to play.").arg(segmentNr),
                           LoggingPriority::Info);

  this->isLocalSource = true;
  this->fileListIt    = this->fileList.begin();
  this->downloadNextFile();
}

void FileDownloader::openURL(QString baseUrl, QString segmentPattern, unsigned segmentNrMax)
{
  DEBUG("Open URL " << baseUrl);
  // We don't check these here. If these don't exist we will get a download error later.
  for (auto i = 0u; i < segmentNrMax; i++)
  {
    auto seg = segmentPattern;
    seg.replace("%i", QString("%1").arg(i));
    auto segmentUrl = baseUrl + seg;
    this->fileList.push_back(segmentUrl);
  }

  this->logger->addMessage(QString("Added %1 remote URLs to file list.").arg(segmentNrMax),
                           LoggingPriority::Info);

  this->isLocalSource = false;
  this->fileListIt    = this->fileList.begin();
  this->downloadNextFile();
}

void FileDownloader::downloadNextFile()
{
  QNetworkAccessManager networkManager;

  this->currentSegment = segmentBuffer->getNextDownloadSegment();

  if (this->isLocalSource)
  {
    QFile inputFile(*this->fileListIt);
    if (!inputFile.open(QIODevice::ReadOnly))
      this->logger->addMessage(QString("Error reading file %1").arg(*this->fileListIt),
                               LoggingPriority::Error);
    else
    {
      // For local files the download finishes immediately
      DEBUG("Loading local file " << *this->fileListIt);
      this->currentSegment->compressedData      = inputFile.readAll();
      this->currentSegment->downloadProgress    = 100.0;
      this->currentSegment->downloadFinished    = true;
      this->currentSegment->compressedSizeBytes = this->currentSegment->compressedData.size();
      emit downloadFinished();
    }
  }
  else
  {
    DEBUG("Start download of file " << *this->fileListIt);

    QNetworkRequest request(*this->fileListIt);
    QNetworkReply * reply = this->networkManager.get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &FileDownloader::updateDownloadProgress);
    this->statusText = "Download";
  }

  this->fileListIt++;
}
