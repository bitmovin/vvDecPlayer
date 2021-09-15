/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <FileDownloader.h>
#include <ManifestFile.h>
#include <SegmentBuffer.h>
#include <common/Frame.h>
#include <common/ILogger.h>
#include <threads/DecoderThread.h>
#include <threads/FileParserThread.h>
#include <threads/FrameConversionThread.h>

#include <QDir>

class PlaybackController
{
public:
  PlaybackController(ILogger *logger);
  ~PlaybackController();

  void reset();

  bool openJsonManifestFile(QString jsonManifestFile);
  bool openPredefinedManifest(unsigned predefinedManifestID);

  void gotoSegment(unsigned segmentNumber);

  QString getStatus();
  auto    getLastSegmentsData() -> std::deque<SegmentData>;

  SegmentBuffer *getSegmentBuffer() { return &this->segmentBuffer; }

private:
  ILogger *logger{};

  std::unique_ptr<FileDownloader>        downloader;
  std::unique_ptr<DecoderThread>         decoder;
  std::unique_ptr<FileParserThread>      parser;
  std::unique_ptr<FrameConversionThread> conversion;

  std::unique_ptr<ManifestFile> manifestFile;

  SegmentBuffer segmentBuffer;
};
