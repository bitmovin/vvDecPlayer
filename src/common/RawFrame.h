/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "typedef.h"
#include <QByteArray>
#include <YUV/YUVPixelFormat.h>

enum class FrameState
{
  Converted,     // Converted to RGB, ready for display
  Decoded,       // Frame is YUV, waiting for conversion to RGB
  Downloaded,    // Waiting for decode
  DownloadQueued // Scheduled for download
};

struct FrameStatus
{
  FrameStatus(FrameState frameState, bool isBeingProcessed)
      : frameState(frameState), isBeingProcessed(isBeingProcessed)
  {
  }
  FrameStatus(FrameState frameState) : frameState(frameState) {}
  FrameState frameState{};
  bool       isBeingProcessed{false};
};

struct RawYUVFrame
{
  QByteArray                    rawData;
  Size                          frameSize;
  YUV_Internals::YUVPixelFormat pixelFormat;
};
