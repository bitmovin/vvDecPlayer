/* MIT License

Copyright (c) 2021 Christian Feldmann <christian.feldmann@gmx.de>
                                      <christian.feldmann@bitmovin.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

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

} // namespace

FileDownloader::FileDownloader(ILogger *logger) : logger(logger)
{
  DEBUG("FileDownloader - Built with SSL version: " << QSslSocket::sslLibraryBuildVersionString());
  DEBUG("FileDownloader - Found SSL library: " << QSslSocket::sslLibraryVersionString());
  DEBUG("FileDownloader - SSL supported: " << QSslSocket::supportsSsl());

  connect(&this->networkManager,
          &QNetworkAccessManager::finished,
          this,
          &FileDownloader::replyFinished);
}

QString FileDownloader::getStatus() const
{
  switch (this->state)
  {
  case State::Idle:
    return "Idle";
  case State::Downloading:
    return "Downloading";
  default:
    return "";
  }
}

size_t FileDownloader::getQueueSize() const { return this->downloadQueue.size(); }

void FileDownloader::replyFinished(QNetworkReply *reply)
{
  DEBUG("Reply finished");
  if (reply->error() != QNetworkReply::NoError)
  {
    DEBUG("Error " << reply->errorString());
    this->logger->addMessage(QString("Download Error: %1").arg(reply->errorString()),
                             LoggingPriority::Error);
    return;
  }

  if (this->state == State::Idle)
  {
    DEBUG("Error - We got a reply but are in idle state");
    this->logger->addMessage("Got not requested download response", LoggingPriority::Error);
    return;
  }
  else
  {
    this->currentSegment->compressedData      = reply->readAll();
    this->currentSegment->downloadProgress    = 100.0;
    this->currentSegment->downloadFinished    = true;
    this->currentSegment->compressedSizeBytes = this->currentSegment->compressedData.size();
    this->currentSegment                      = nullptr;
    emit downloadOfSegmentFinished();
  }

  this->state = State::Idle;
  this->tryStartOfNextDownload();
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

void FileDownloader::addFileToDownloadQueue(Segment *segment)
{
  this->downloadQueue.push(segment);
  this->tryStartOfNextDownload();
}

void FileDownloader::tryStartOfNextDownload()
{
  if (this->downloadQueue.empty())
  {
    DEBUG("No more downloads in queue - Idle");
    this->state = State::Idle;
    return;
  }

  if (this->state == State::Downloading)
    return;

  this->currentSegment = this->downloadQueue.front();
  this->downloadQueue.pop();

  auto url = this->currentSegment->segmentInfo.downloadUrl;
  if (isURLLocalFile(url))
  {
    QFile inputFile(url);
    if (!inputFile.open(QIODevice::ReadOnly))
      this->logger->addMessage(QString("Error reading file %1").arg(url), LoggingPriority::Error);
    else
    {
      // For local files the download finishes immediately
      DEBUG("Loading local file " << url);
      this->currentSegment->compressedData      = inputFile.readAll();
      this->currentSegment->downloadProgress    = 100.0;
      this->currentSegment->downloadFinished    = true;
      this->currentSegment->compressedSizeBytes = this->currentSegment->compressedData.size();
      emit downloadOfSegmentFinished();
      this->tryStartOfNextDownload();
    }
  }
  else
  {
    DEBUG("Start download of file " << url);

    QNetworkRequest request(url);
    QNetworkReply * reply = this->networkManager.get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &FileDownloader::updateDownloadProgress);
    this->state = State::Downloading;
  }
}
