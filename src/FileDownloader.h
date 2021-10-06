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

  enum class State
  {
    Idle,
    DownloadSegment,
    DownloadHighestResolutionSPS
  };
  State state{State::Idle};

  enum class FileType
  {
    NextSegment,
    HighestRepresentationSPS
  };
  void downloadFile(FileType fileType);

  SegmentBuffer::SegmentPtr currentSegment;

  bool isLocalSource{false};

  QNetworkAccessManager networkManager;
};
