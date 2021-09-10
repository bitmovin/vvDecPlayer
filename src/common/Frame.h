/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "typedef.h"
#include <QByteArray>
#include <QImage>
#include <YUV/YUVPixelFormat.h>

enum class FrameState
{
  ConvertedToRGB, // Ready for display (rgbImage filled)
  Decoded,        // Frame is YUV, waiting for conversion to RGB (rawYUVData set)
  Empty
};

struct Frame
{
  FrameState frameState{FrameState::Empty};

  QByteArray                    rawYUVData;
  Size                          frameSize;
  YUV_Internals::YUVPixelFormat pixelFormat;

  size_t   nrBytesCompressed{};
  unsigned poc{};

  QImage rgbImage;
};
