/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "DecoderManager.h"
#include "FileDownloader.h"
#include "FrameConversionThread.h"
#include "ILogger.h"
#include <SegmentBuffer.h>
#include <common/Frame.h>


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

  QString getStatus();
  auto    getLastSegmentsData() -> std::deque<SegmentData>;

public slots:
  void onSegmentReadyForDecode();
  void onDecodeOfSegmentDone();

private:
  ILogger *logger{};

  std::unique_ptr<FileDownloader>        downloader;
  std::unique_ptr<DecoderManager>        decoder;
  std::unique_ptr<FrameConversionThread> conversion;

  SegmentBuffer segmentBuffer;
};
