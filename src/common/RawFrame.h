/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "typedef.h"
#include <QByteArray>
#include <YUV/YUVPixelFormat.h>

struct RawYUVFrame
{
  QByteArray                    rawData;
  Size                          frameSize;
  YUV_Internals::YUVPixelFormat pixelFormat;
};
