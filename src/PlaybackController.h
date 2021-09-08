/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "DecoderThread.h"
#include "FileDownloader.h"
#include "FileParserThread.h"
#include "FrameConversionThread.h"
#include "ILogger.h"
#include <SegmentBuffer.h>
#include <common/Frame.h>

#include <QDir>

class PlaybackController
{
public:
  PlaybackController(ILogger *logger);
  ~PlaybackController();

  void reset();

  void openDirectory(QDir path, QString segmentPattern);
  void openURL(QString url, QString segmentPattern, unsigned segmentNrMax);

  QString getStatus();
  auto    getLastSegmentsData() -> std::deque<SegmentData>;

  SegmentBuffer *getSegmentBuffer() { return &this->segmentBuffer; }

private:
  ILogger *logger{};

  std::unique_ptr<FileDownloader>        downloader;
  std::unique_ptr<DecoderThread>         decoder;
  std::unique_ptr<FileParserThread>      parser;
  std::unique_ptr<FrameConversionThread> conversion;

  SegmentBuffer segmentBuffer;
};
