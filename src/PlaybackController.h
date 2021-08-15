/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

#include "ViewWidget.h"

class PlaybackController
{
public:
  PlaybackController(ViewWidget *viewWidget);

private:
  ViewWidget *viewWidget {};
};
