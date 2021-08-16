/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <common/File.h>
#include <condition_variable>
#include <decoder/decoderBase.h>
#include <thread>

#include <QObject>

class DecoderManager : public QObject
{
  Q_OBJECT

public:
  DecoderManager(ILogger *logger);
  ~DecoderManager();

  bool isDecodeRunning();
  void decodeFile(std::shared_ptr<File> file);

signals:
  void onDecodeOfSegmentDone();

private:
  ILogger *logger{};

  void runDecoder();

  std::shared_ptr<File> currentFile;
  std::mutex            currentFileMutex;

  std::unique_ptr<decoder::decoderBase> decoder;

  std::thread             decoderThread;
  std::condition_variable decoderCV;
  bool                    decoderAbort{false};
};
