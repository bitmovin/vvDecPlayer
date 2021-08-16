/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "FileDownloadManager.h"
#include "ILogger.h"
#include <decoder/decoderBase.h>


#include <QDir>
#include <QObject>

class PlaybackController : public QObject
{
  Q_OBJECT

public:
  PlaybackController(ILogger *logger);

  void reset();

  void openDirectory(QDir path, QString segmentPattern);

public slots:
  void onSegmentReadyForDecode();

private:
  ILogger *logger{};

  std::unique_ptr<decoder::decoderBase> decoder;
  
  std::unique_ptr<FileDownloadManager>  fileDownloadManager;
};
