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

#include "typedef.h"
#include <QByteArray>
#include <QImage>
#include <video/PixelFormatYUV.h>

enum class FrameState
{
  ConvertedToRGB, // Ready for display (rgbImage filled)
  Decoded,        // Frame is YUV, waiting for conversion to RGB (rawYUVData set)
  Empty
};

class Frame
{
public:
  Frame() = default;

  void clear()
  {
    this->frameState        = FrameState::Empty;
    this->frameSize         = {};
    this->pixelFormat       = {};
    this->nrBytesCompressed = 0;
    this->poc               = 0;
  }

  FrameState frameState{FrameState::Empty};

  QByteArray                 rawYUVData;
  Size                       frameSize{};
  video::yuv::PixelFormatYUV pixelFormat{};

  size_t   nrBytesCompressed{0};
  unsigned poc{0};

  QImage rgbImage;
};
