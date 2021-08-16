/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

#include "ViewWidget.h"
#include <decoder/decoderBase.h>

#include <QDir>

class PlaybackController
{
public:
  PlaybackController(ViewWidget *viewWidget);

  void reset();

  void openDirectory(QDir path, QString segmentPattern);

private:
  ViewWidget *viewWidget {};

  std::unique_ptr<decoder::decoderBase> decoder;

  std::vector<QString> localFileList;

  QString segmentPattern;
};
