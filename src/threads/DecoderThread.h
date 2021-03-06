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

#include <SegmentBuffer.h>
#include <common/ILogger.h>
#include <decoder/decoderBase.h>

#include <QObject>
#include <condition_variable>
#include <optional>
#include <thread>

class DecoderThread : public QObject
{
  Q_OBJECT

public:
  DecoderThread(ILogger *logger, SegmentBuffer *segmentBuffer);
  ~DecoderThread();
  void abort();

  void setOpenGopAdaptiveResolutionChange(bool adaptiveResolutioChange);

  QString getStatus() const;

public slots:
  void onDownloadOfFirstSPSSegmentFinished(QByteArray segmentData);

private:
  ILogger *      logger{};
  SegmentBuffer *segmentBuffer{};

  void runDecoder();

  std::unique_ptr<decoder::decoderBase> decoder;

  std::thread decoderThread;
  bool        decoderAbort{};
  bool        adaptiveResolutioChange{};

  QByteArray highestRenditionSPS;

  QString statusText;
};
