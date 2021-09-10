/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <QNetworkAccessManager>
#include <SegmentBuffer.h>
#include <common/Segment.h>
#include <common/typedef.h>

#include <QDir>
#include <deque>
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
  FileDownloader(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~FileDownloader() = default;

  void openDirectory(QDir path, QString segmentPattern);
  void openURL(QString baseUrl, QString segmentPattern, unsigned segmentNrMax);

  void gotoSegment(unsigned segmentNumber) { this->segmentNumber = segmentNumber; }

  QString getStatus() const;

signals:
  void downloadFinished();

private slots:
  void downloadNextFile();

  void replyFinished(QNetworkReply *reply);
  void updateDownloadProgress(int64_t val, int64_t max);

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  SegmentBuffer::SegmentPtr currentSegment;

  std::vector<QString> fileList;
  unsigned             segmentNumber{};

  bool isLocalSource{false};

  QString statusText;

  QNetworkAccessManager networkManager;
};
