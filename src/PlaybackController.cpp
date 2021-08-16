/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "PlaybackController.h"

#include <decoder/decoderVVDec.h>

PlaybackController::PlaybackController(ViewWidget *viewWidget) : viewWidget(viewWidget)
{
  assert(viewWidget != nullptr);
  this->reset();
}

void PlaybackController::reset()
{
  this->viewWidget->clearMessages();

  this->decoder = std::make_unique<decoder::decoderVVDec>();
  if (this->decoder->errorInDecoder())
  {
    this->viewWidget->addMessage("Error in decoder: " + this->decoder->decoderErrorString(),
                                 ViewWidget::MessagePriority::Error);
  }

  this->viewWidget->addMessage("Playback Controller initialized",
                               ViewWidget::MessagePriority::Info);

  this->localFileList.clear();
  this->segmentPattern = {};
}

void PlaybackController::openDirectory(QDir path, QString segmentPattern)
{
  if (!this->localFileList.empty())
    this->reset();

  this->segmentPattern = segmentPattern;

  unsigned segmentNr = 0;
  while (true)
  {
    auto file = segmentPattern;
    file.replace("%i", QString("%1").arg(segmentNr));
    if (!path.exists(file))
      break;

    auto fullFilePath = path.filePath(file);
    this->localFileList.push_back(fullFilePath);

    segmentNr++;
  }

  this->viewWidget->addMessage(QString("Found %1 local files to play.").arg(segmentNr),
                               ViewWidget::MessagePriority::Info);
}
