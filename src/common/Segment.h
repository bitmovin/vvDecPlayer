/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "Frame.h"

#include <QString>
#include <QByteArray>

struct Segment
{
  QString     pathOrURL;
  
  QByteArray  compressedData;
  std::size_t compressedSizeBytes{};
  bool        isLocalFile{};
  using Percent = double;
  Percent downloadProgress{0.0};

  std::vector<Frame> frames;
};
