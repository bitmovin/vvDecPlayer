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
  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    constexpr size_t MAX_SEGMENTS_IN_BUFFER = 6;
    return this->segments.size() < MAX_SEGMENTS_IN_BUFFER;
  });

  DEBUG("Push segment");
  this->segments.push_back(segment);

  this->eventCV.notify_all();
}

SegmentBuffer::SegmentIt SegmentBuffer::getFirstSegmentToDecode()
{
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() { return this->segments.size() > 0; });

  return this->segments.begin();
}

SegmentBuffer::SegmentIt SegmentBuffer::getNextSegmentToDecode(SegmentIt segmentIt)
{
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentIt]() {
    auto nextIt = segmentIt;
    nextIt++;
    return nextIt != this->segments.end();
  });

  return segmentIt++;
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToConvert()
{
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    return this->segments.size() > 0 && this->segments.front().frames.size() > 0;
  });

  return this->begin();
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToConvert(FrameIterator frameIt)
{
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, frameIt]() {
    auto nextFrame = frameIt;
    nextFrame++;
    return nextFrame != this->end();
  });

  return frameIt++;
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToDisplay()
{
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

  return this->begin();
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToDisplay(FrameIterator frameIt)
{
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, frameIt]() {
    auto nextFrame = frameIt;
    nextFrame++;
    return nextFrame != this->end() && nextFrame->frameState == FrameState::ConvertedToRGB;
  });

  return frameIt++;
}