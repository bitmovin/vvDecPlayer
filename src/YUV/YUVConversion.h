/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "YUVPixelFormat.h"

#include <QByteArray>
#include <QImage>

// Convert from YUV (which ever format is selected) to image (RGB-888)
void convertYUVToImage(const QByteArray &                   sourceBuffer,
                       QImage &                             outputImage,
                       const YUV_Internals::YUVPixelFormat &yuvFormat,
                       const Size &                         curFrameSize);
