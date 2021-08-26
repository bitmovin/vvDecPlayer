/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "SegmentBuffer.h"

#define DEBUG_SEGMENT_BUFFER 0
#if DEBUG_SEGMENT_BUFFER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

SegmentBuffer::SegmentPtr getNextSegmentFromQueue(SegmentBuffer::SegmentPtr          curSegment,
                                                  const SegmentBuffer::SegmentDeque &segments)
{
  auto segmentIt = std::find(segments.begin(), segments.end(), curSegment);
  if (segmentIt == segments.end())
    return {};
  segmentIt++;
  if (segmentIt == segments.end())
    return {};
  return *segmentIt;
}

SegmentBuffer::FrameIterator getNextFrame(const SegmentBuffer::FrameIterator &frame,
                                          const SegmentBuffer::SegmentDeque & segments)
{
  auto segmentIt = std::find(segments.begin(), segments.end(), frame.segment);
  if (segmentIt == segments.end())
    return {};
  auto &segmentFrames  = (*segmentIt)->frames;
  auto  segmentFrameIt = std::find(segmentFrames.begin(), segmentFrames.end(), frame.frame);
  if (segmentFrameIt == segmentFrames.end())
    return {};
  auto nextFrameInSegment = (++segmentFrameIt);
  if (nextFrameInSegment == segmentFrames.end())
  {
    auto nextSegment = (++segmentIt);
    if (nextSegment == segments.end())
      return {};
    if ((*nextSegment)->frames.size() == 0)
      return {};
    return {*nextSegment, (*nextSegment)->frames.front()};
  }
  return {frame.segment, *nextFrameInSegment};
}

} // namespace

SegmentBuffer::~SegmentBuffer() { this->abort(); }

void SegmentBuffer::abort()
{
  DEBUG("SegmentBuffer: Abort");
  this->aborted = true;
  this->eventCV.notify_all();
}

std::vector<SegmentBuffer::SegmentRenderInfo>
SegmentBuffer::getBufferStatusForRender(FramePt curPlaybackFrame)
{
  std::unique_lock               lk(this->segmentQueueMutex);
  std::vector<SegmentRenderInfo> states;
  for (auto &segment : this->segments)
  {
    SegmentRenderInfo segmentState;
    segmentState.downloadProgress = segment->downloadProgress;
    segmentState.nrFrames         = segment->neFrames;
    unsigned frameCounter         = 0;
    for (auto &frame : segment->frames)
    {
      segmentState.frameStates.push_back(frame->frameState);
      if (frame == curPlaybackFrame)
        segmentState.indexOfCurFrameInFrames = frameCounter;
      frameCounter++;
    }
    states.push_back(segmentState);
  }
  return states;
}

void SegmentBuffer::pushDownloadedSegment(SegmentPtr segment)
{
  DEBUG("SegmentBuffer: Try push downloaded segment");
  std::unique_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    if (this->aborted)
      return true;
    constexpr size_t MAX_SEGMENTS_IN_BUFFER = 6;
    return this->segments.size() < MAX_SEGMENTS_IN_BUFFER;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Not Pushed downloaded segment because of abort");
    return;
  }

  DEBUG("SegmentBuffer: Pushed downloaded segment");
  this->segments.push_back(segment);

  this->eventCV.notify_all();
}

SegmentBuffer::SegmentPtr SegmentBuffer::getFirstSegmentToDecode()
{
  DEBUG("SegmentBuffer: Waiting for first segment to decode.");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() { return this->aborted || this->segments.size() > 0; });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First segment to decode not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: First segment to decode ready");
  return *this->segments.begin();
}

SegmentBuffer::SegmentPtr SegmentBuffer::getNextSegmentToDecode(SegmentPtr segmentPtr)
{
  DEBUG("SegmentBuffer: Waiting for next segment to decode");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentPtr]() {
    if (this->aborted)
      return true;
    auto nextSegment = getNextSegmentFromQueue(segmentPtr, this->segments);
    return bool(nextSegment);
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next segment to decode not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: Next segment to decode ready");
  return getNextSegmentFromQueue(segmentPtr, this->segments);
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToConvert()
{
  DEBUG("SegmentBuffer: Waiting for first frame to convert");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    if (this->aborted)
      return true;
    auto frameAvailabe = this->segments.size() > 0 && this->segments.front()->frames.size() > 0;
    return frameAvailabe;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First frame to convert not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: First frame to convert ready");
  return {this->segments.front(), this->segments.front()->frames.front()};
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToConvert(FrameIterator frameIt)
{
  assert(!frameIt.isNull());
  DEBUG("Waiting for next frame to convert.");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, frameIt]() {
    if (this->aborted)
      return true;
    auto nextFrame = getNextFrame(frameIt, this->segments);
    return !nextFrame.isNull();
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next frame to convert not ready because of abort");
    return {};
  }

  DEBUG("Next frame to convert ready.");
  return getNextFrame(frameIt, this->segments);
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToDisplay()
{
  std::shared_lock lk(this->segmentQueueMutex);

  if (this->aborted)
  {
    DEBUG("SegmentBuffer:: First frame to display not ready because aborted");
    return {};
  }

  if (this->segments.size() == 0)
  {
    DEBUG("SegmentBuffer:: First frame to display not ready yet");
    return {};
  }

  auto &firstSegment = this->segments.front();
  if (firstSegment->frames.size() == 0 ||
      firstSegment->frames.front()->frameState != FrameState::ConvertedToRGB)
  {
    DEBUG("SegmentBuffer:: First frame to display not ready yet");
    return {};
  }

  DEBUG("First frame to display ready.");
  this->eventCV.notify_all();
  return {firstSegment, firstSegment->frames.front()};
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToDisplay(FrameIterator frameIt)
{
  assert(!frameIt.isNull());
  SegmentBuffer::FrameIterator nextFrame;
  {
    std::shared_lock lk(this->segmentQueueMutex);

    if (this->aborted)
    {
      DEBUG("SegmentBuffer:: Next frame to display not ready because aborted");
      return {};
    }

    nextFrame = getNextFrame(frameIt, this->segments);
    if (nextFrame.isNull() || nextFrame.frame->frameState != FrameState::ConvertedToRGB)
    {
      DEBUG("SegmentBuffer:: Next frame to display not ready yet");
      return {};
    }
  }

  if (frameIt.segment != nextFrame.segment)
  {
    std::unique_lock lk(this->segmentQueueMutex);
    assert(frameIt.segment == this->segments.front());
    this->segments.pop_front();
  }

  DEBUG("Next frame to display ready.");
  this->eventCV.notify_all();
  return nextFrame;
}
