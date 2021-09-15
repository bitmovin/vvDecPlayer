/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <ManifestFile.h>
#include <SegmentBuffer.h>
#include <common/ILogger.h>
#include <common/Segment.h>
#include <common/typedef.h>

#include <QDir>
#include <QNetworkAccessManager>
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

  void activateManifest(ManifestFile *manifestFile);

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
  ManifestFile * manifestFile{};

  SegmentBuffer::SegmentPtr currentSegment;

  bool isLocalSource{false};

  QString statusText;

  QNetworkAccessManager networkManager;
};
