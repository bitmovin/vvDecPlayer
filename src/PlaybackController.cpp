/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

PlaybackController::PlaybackController(ViewWidget *viewWidget) : viewWidget(viewWidget)
{
  assert(viewWidget != nullptr);

  

  this->viewWidget->addMessage("Playback Controller initialized", ViewWidget::MessagePriority::Info);
}
