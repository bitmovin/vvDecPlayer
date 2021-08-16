/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <common/File.h>
#include <condition_variable>
#include <decoder/decoderBase.h>
#include <thread>
#include <optional>
#include <QObject>
#include <FrameConversionBuffer.h>

class DecoderManager : public QObject
{
  Q_OBJECT

public:
  DecoderManager(ILogger *logger, FrameConversionBuffer *frameConversionBuffer);
  ~DecoderManager();

  bool isDecodeRunning();
  void decodeFile(std::shared_ptr<File> file);

signals:
  void onDecodeOfSegmentDone();

private:
  ILogger *logger{};
  FrameConversionBuffer *frameConversionBuffer{};

  void runDecoder();

  std::optional<std::size_t> findNextNalInCurFile(std::size_t start);

  std::shared_ptr<File> currentFile;
  std::mutex            currentFileMutex;
  std::size_t           currentDataOffset{};

  std::unique_ptr<decoder::decoderBase> decoder;

  std::thread             decoderThread;
  std::condition_variable decoderCV;
  bool                    decoderAbort{false};
};
