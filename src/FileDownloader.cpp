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

namespace
{

bool isURLLocalFile(QString url)
{
  return !url.startsWith("https://") && !url.startsWith("http://");
}

}

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

QString FileDownloader::getStatus() const { return this->statusText; }

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

void FileDownloader::activateManifest(ManifestFile *manifestFile)
{
  this->manifestFile = manifestFile;
  this->downloadNextFile();
}

void FileDownloader::downloadNextFile()
{
  QNetworkAccessManager networkManager;

  auto manifestSegment = this->manifestFile->getNextSegment();

  this->currentSegment                = segmentBuffer->getNextDownloadSegment();
  this->currentSegment->segmentNumber = manifestSegment.segmentNumber;

  auto url = manifestSegment.downloadUrl;
  if (isURLLocalFile(url))
  {
    QFile inputFile(url);
    if (!inputFile.open(QIODevice::ReadOnly))
      this->logger->addMessage(QString("Error reading file %1").arg(url),
                               LoggingPriority::Error);
    else
    {
      // For local files the download finishes immediately
      DEBUG("Loading local file " << url);
      this->currentSegment->compressedData      = inputFile.readAll();
      this->currentSegment->downloadProgress    = 100.0;
      this->currentSegment->downloadFinished    = true;
      this->currentSegment->compressedSizeBytes = this->currentSegment->compressedData.size();
      emit downloadFinished();
    }
  }
  else
  {
    DEBUG("Start download of file " << url);

    QNetworkRequest request(url);
    QNetworkReply * reply = this->networkManager.get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &FileDownloader::updateDownloadProgress);
    this->statusText = "Download";
  }
}
