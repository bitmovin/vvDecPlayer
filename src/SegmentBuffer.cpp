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

SegmentBuffer::~SegmentBuffer()
{
  this->abort();
}

void SegmentBuffer::abort()
{
  DEBUG("SegmentBuffer: Abort");
  this->aborted = true;
  this->eventCV.notify_all();
}

SegmentBuffer::FrameIterator SegmentBuffer::begin()
{
  return SegmentBuffer::FrameIterator(
      this->segments.begin(), this->segments.front().frames.begin(), &this->segments);
}
SegmentBuffer::FrameIterator SegmentBuffer::end()
{
  return SegmentBuffer::FrameIterator(this->segments.end(), {}, &this->segments);
}

void SegmentBuffer::pushDownloadedSegment(Segment segment)
{
  DEBUG("SegmentBuffer: Try push downloaded segment");
  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
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

SegmentBuffer::SegmentIt SegmentBuffer::getFirstSegmentToDecode()
{
  DEBUG("SegmentBuffer: Waiting for first segment to decode.");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() { return this->aborted || this->segments.size() > 0; });

  DEBUG("SegmentBuffer: First segment to decode ready.");
  return this->segments.begin();
}

SegmentBuffer::SegmentIt SegmentBuffer::getNextSegmentToDecode(SegmentIt segmentIt)
{
  DEBUG("SegmentBuffer: Waiting for next segment to decode");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentIt]() {
    if (this->aborted)
      return true;
    auto nextIt = segmentIt;
    nextIt++;
    return nextIt != this->segments.end();
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next segment to decode not ready because of abort");
    return this->segments.end();
  }

  DEBUG("SegmentBuffer: Next segment to decode ready");
  return segmentIt++;
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToConvert()
{
  DEBUG("SegmentBuffer: Waiting for first frame to convert");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    auto frameAvailabe = this->segments.size() > 0 && this->segments.front().frames.size() > 0;
    return this->aborted || frameAvailabe;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First frame to convert not ready because of abort");
    return this->end();
  }

  DEBUG("SegmentBuffer: First frame to convert ready");
  return this->begin();
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToConvert(FrameIterator frameIt)
{
  DEBUG("Waiting for next frame to convert.");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, frameIt]() {
    auto nextFrame = frameIt;
    nextFrame++;
    return nextFrame != this->end();
  });

  DEBUG("Next frame to convert ready.");
  return frameIt++;
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToDisplay()
{
  DEBUG("Waiting for first frame to display.");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    if (this->segments.size() == 0)
      return false;
    auto &segment = this->segments.front();
    if (segment.frames.size() == 0)
      return false;
    auto &frame = segment.frames.front();
    return frame.frameState == FrameState::ConvertedToRGB;
  });

  DEBUG("First frame to display ready.");
  return this->begin();
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToDisplay(FrameIterator frameIt)
{
  DEBUG("Waiting for Next frame to display.");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, frameIt]() {
    auto nextFrame = frameIt;
    nextFrame++;
    return nextFrame != this->end() && nextFrame->frameState == FrameState::ConvertedToRGB;
  });

  DEBUG("Next frame to display ready.");
  return frameIt++;
}