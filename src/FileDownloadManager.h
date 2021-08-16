/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "FileDownloader.h"
#include "ILogger.h"

#include <QDir>
#include <QObject>

class FileDownloadManager : public QObject
{
  Q_OBJECT

public:
  FileDownloadManager(ILogger *logger);

  void                  openDirectory(QDir path, QString segmentPattern);
  std::shared_ptr<File> getNextDownloadedFile();
  bool                  isDownloadRunning() { return this->fileDownloader->isDownloadRunning(); }

signals:
  void onSegmentReadyForDecode();

private slots:
  void onDownloadDone();

private:
  ILogger *logger{};

  void startDownloadOfNextFile();

  std::vector<QString>           localFileList;
  std::vector<QString>::iterator currentFileIt;

  std::unique_ptr<FileDownloader> fileDownloader;
};
