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
 * This class works async. You just push files to download into a queue and
 * the downloader will signal every time a download is done.
 */
class FileDownloader : public QObject
{
  Q_OBJECT

public:
  FileDownloader(ILogger *logger);
  ~FileDownloader() = default;

  QString getStatus() const;
  size_t  getQueueSize() const;

  struct DownloadInfo
  {
  };

public slots:
  void addFileToDownloadQueue(Segment *segment);

signals:
  void downloadOfSegmentFinished();

private slots:
  void replyFinished(QNetworkReply *reply);
  void updateDownloadProgress(int64_t val, int64_t max);

private:
  ILogger *logger{};

  enum class State
  {
    Idle,
    Downloading
  };
  State state{State::Idle};

  Segment *currentSegment{};

  bool isLocalSource{false};

  QNetworkAccessManager networkManager;

  std::queue<Segment *> downloadQueue;
  void                  tryStartOfNextDownload();
};
