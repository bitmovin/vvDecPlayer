/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include "FileDownloader.h"

#include <QObject>
#include <QDir>

class FileDownloadManager : public QObject
{
  Q_OBJECT

public:
  FileDownloadManager(ILogger *logger);

  void openDirectory(QDir path, QString segmentPattern);

signals:
  void onSegmentReadyForDecode();

private slots:
  void onDownloadDone();

private:
  ILogger *logger{};

  std::vector<QString> localFileList;
  std::vector<QString>::iterator currentFileIt;

  std::unique_ptr<FileDownloader>       fileDownloader;
};
