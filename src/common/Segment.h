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

#include "Frame.h"

#include <QByteArray>
#include <QString>
#include <memory>

class Segment
{
public:
  Segment()  = default;
  ~Segment() = default;

  void clear()
  {
    this->segmentInfo         = {};
    this->compressedSizeBytes = 0;
    this->downloadProgress    = 0.0;
    this->downloadFinished    = false;
    this->parsingFinished     = false;
    this->nrFrames            = 0;
    this->frames.clear();
  }

  struct SegmentInfo
  {
    unsigned segmentNumber{};
    unsigned rendition{};
    QString  downloadUrl;
    double   fps{};
  };
  SegmentInfo segmentInfo{};

  QByteArray  compressedData;
  std::size_t compressedSizeBytes{0};
  bool        isLocalFile{};

  using Percent = double;
  Percent downloadProgress{0.0};
  bool    downloadFinished{false};
  bool    parsingFinished{false};

  unsigned nrFrames{0};

  std::vector<std::unique_ptr<Frame>> frames;
};
