/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "Frame.h"

#include <QByteArray>
#include <QString>

struct Segment
{
  QString pathOrURL;
  unsigned segmentNumber{};

  QByteArray  compressedData;
  std::size_t compressedSizeBytes{};
  bool        isLocalFile{};

  using Percent = double;
  Percent downloadProgress{0.0};
  bool    downloadFinished{false};
  bool    parsingFinished{false};

  unsigned nrFrames{24};

  std::vector<std::shared_ptr<Frame>> frames;
};
