/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "ILogger.h"
#include <FrameConversionBuffer.h>
#include <QObject>
#include <common/File.h>
#include <condition_variable>
#include <decoder/decoderBase.h>
#include <optional>
#include <thread>


class DecoderManager : public QObject
{
  Q_OBJECT

public:
  DecoderManager(ILogger *logger, FrameConversionBuffer *frameConversionBuffer);
  ~DecoderManager();
  void abort();

  bool isDecodeRunning();
  void decodeFile(std::shared_ptr<File> file);

  QString getStatus();
  void addFrameQueueInfo(std::vector<FrameStatus> &info);

signals:
  void onDecodeOfSegmentDone();
  void onSegmentLengthUpdate(unsigned segmentLength);

private:
  ILogger *              logger{};
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

  size_t currentDecodeFrame{};

  enum class DecoderState
  {
    Running,
    WaitingToPushOutput,
    WaitingForNextFile
  };
  DecoderState decoderState{DecoderState::Running};

  // Note: This is just a guess. After decoding one segment we will know.
  unsigned segmentLength{24};
};
