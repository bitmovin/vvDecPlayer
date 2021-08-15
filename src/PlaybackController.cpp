/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

#include <decoder/decoderVVDec.h>

PlaybackController::PlaybackController(ViewWidget *viewWidget) : viewWidget(viewWidget)
{
  assert(viewWidget != nullptr);

  this->decoder = std::make_unique<decoder::decoderVVDec>();
  if (this->decoder->errorInDecoder())
  {
    this->viewWidget->addMessage("Error in decoder: " + this->decoder->decoderErrorString(),
                                 ViewWidget::MessagePriority::Error);
  }

  this->viewWidget->addMessage("Playback Controller initialized",
                               ViewWidget::MessagePriority::Info);
}
