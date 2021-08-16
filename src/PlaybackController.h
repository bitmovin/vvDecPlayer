/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

#include "ViewWidget.h"
#include <decoder/decoderBase.h>

class PlaybackController
{
public:
  PlaybackController(ViewWidget *viewWidget);

  void reset();

private:
  ViewWidget *viewWidget {};

  std::unique_ptr<decoder::decoderBase> decoder;
};
