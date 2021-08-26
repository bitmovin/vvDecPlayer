/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "Frame.h"

#include <QByteArray>
#include <QString>

struct Segment
{
  QString pathOrURL;

  QByteArray  compressedData;
  std::size_t compressedSizeBytes{};
  bool        isLocalFile{};

  using Percent = double;
  Percent downloadProgress{0.0};

  unsigned neFrames{24};

  std::vector<std::shared_ptr<Frame>> frames;
};
