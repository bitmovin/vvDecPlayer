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

#include <FileDownloader.h>
#include <ManifestFile.h>
#include <SegmentBuffer.h>
#include <common/Frame.h>
#include <common/ILogger.h>
#include <threads/DecoderThread.h>
#include <threads/FileParserThread.h>
#include <threads/FrameConversionThread.h>

#include <QDir>
#include <QObject>

class PlaybackController : public QObject
{
Q_OBJECT

public:
  PlaybackController(ILogger *logger);
  ~PlaybackController();

  void reset();

  bool openJsonManifestFile(QString jsonManifestFile);
  bool openPredefinedManifest(unsigned predefinedManifestID);

  void gotoSegment(unsigned segmentNumber);
  void increaseRendition();
  void decreaseRendition();

  QString getStatus();
  auto    getLastSegmentsData() -> std::deque<SegmentData>;

  SegmentBuffer *getSegmentBuffer() { return &this->segmentBuffer; }
  ManifestFile * getManifest() { return this->manifestFile.get(); }

private:
  ILogger *logger{};

  std::unique_ptr<FileDownloader>        downloader;
  std::unique_ptr<DecoderThread>         decoder;
  std::unique_ptr<FileParserThread>      parser;
  std::unique_ptr<FrameConversionThread> conversion;

  std::unique_ptr<ManifestFile> manifestFile;

  SegmentBuffer segmentBuffer;
};
