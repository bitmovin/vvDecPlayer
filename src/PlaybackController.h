/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "DecoderManager.h"
#include "FileDownloadManager.h"
#include "FrameConversionBuffer.h"
#include "ILogger.h"
#include <common/RawFrame.h>

#include <QDir>
#include <QObject>

class PlaybackController : public QObject
{
  Q_OBJECT

public:
  PlaybackController(ILogger *logger);
  ~PlaybackController();

  void reset();

  void openDirectory(QDir path, QString segmentPattern);

  FrameConversionBuffer *getFrameConversionBuffer() { return this->frameConversionBuffer.get(); }
  
  QString getStatus();
  std::vector<FrameStatus> getFrameQueueInfo();

public slots:
  void onSegmentReadyForDecode();
  void onDecodeOfSegmentDone();

private:
  ILogger *logger{};

  std::unique_ptr<FileDownloadManager>   fileDownloadManager;
  std::unique_ptr<DecoderManager>        decoderManager;
  std::unique_ptr<FrameConversionBuffer> frameConversionBuffer;
};
