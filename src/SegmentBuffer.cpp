/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "SegmentBuffer.h"

#define DEBUG_SEGMENT_BUFFER 1
#if DEBUG_SEGMENT_BUFFER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

SegmentBuffer::SegmentPtr getNextSegmentFromQueue(SegmentBuffer::SegmentPtr    curSegment,
                                                  SegmentBuffer::SegmentDeque *segmentDeque)
{
  for (auto it = segmentDeque->begin(); it != segmentDeque->end(); it++)
  {
    if (*it == curSegment)
    {
      auto nextSegment = it++;
      if (nextSegment != segmentDeque->end())
        return *it;
      break;
    }
  }
  return {};
}

} // namespace

SegmentBuffer::FrameIterator::FrameIterator(const FrameIterator &it)
    : curSegment(it.curSegment), frameIt(it.frameIt), segments(it.segments)
{
}

SegmentBuffer::FrameIterator::FrameIterator(SegmentPtr    segment,
                                            FrameIt       frameIt,
                                            SegmentDeque *segments)
    : curSegment(segment), frameIt(frameIt), segments(segments)
{
}

SegmentBuffer::FrameIterator &SegmentBuffer::FrameIterator::operator++()
{
  this->frameIt++;
  if (this->frameIt == this->curSegment->frames.end())
  {
    auto nextSegment = getNextSegmentFromQueue(this->curSegment, this->segments);
    if (!nextSegment || nextSegment->frames.size() == 0)
    {
      this->curSegment.reset();
      this->segments = nullptr;
    }
    else
    {
      this->curSegment = nextSegment;
      this->frameIt = nextSegment->frames.begin();
    }
  }
  return *this;
}

SegmentBuffer::FrameIterator SegmentBuffer::FrameIterator::operator++(int)
{
  FrameIterator tmp = *this;
  ++(*this);
  return tmp;
}

SegmentBuffer::~SegmentBuffer() { this->abort(); }

void SegmentBuffer::abort()
{
  DEBUG("SegmentBuffer: Abort");
  this->aborted = true;
  this->eventCV.notify_all();
}

SegmentBuffer::FrameIterator SegmentBuffer::begin()
{
  assert(this->segments.size() > 0);
  assert(this->segments.front()->frames.size() > 0);
  return SegmentBuffer::FrameIterator(
      *this->segments.begin(), this->segments.front()->frames.begin(), &this->segments);
}
SegmentBuffer::FrameIterator SegmentBuffer::end() { return {}; }

void SegmentBuffer::pushDownloadedSegment(SegmentPtr segment)
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

SegmentBuffer::SegmentPtr SegmentBuffer::getFirstSegmentToDecode()
{
  DEBUG("SegmentBuffer: Waiting for first segment to decode.");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() { return this->aborted || this->segments.size() > 0; });

  DEBUG("SegmentBuffer: First segment to decode ready.");
  return *this->segments.begin();
}

SegmentBuffer::SegmentPtr SegmentBuffer::getNextSegmentToDecode(SegmentPtr segmentPtr)
{
  DEBUG("SegmentBuffer: Waiting for next segment to decode");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentPtr]() {
    if (this->aborted)
      return true;
    auto nextSegment = getNextSegmentFromQueue(segmentPtr, &this->segments);
    return bool(nextSegment);
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next segment to decode not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: Next segment to decode ready");
  return getNextSegmentFromQueue(segmentPtr, &this->segments);
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToConvert()
{
  DEBUG("SegmentBuffer: Waiting for first frame to convert");
  this->eventCV.notify_all();

  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    auto frameAvailabe = this->segments.size() > 0 && this->segments.front()->frames.size() > 0;
    return this->aborted || frameAvailabe;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First frame to convert not ready because of abort");
    return {};
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
    return nextFrame.isNull();
  });

  DEBUG("Next frame to convert ready.");
  return frameIt++;
}

SegmentBuffer::FrameIterator SegmentBuffer::getFirstFrameToDisplay()
{
  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);

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
      firstSegment->frames.front().frameState == FrameState::ConvertedToRGB)
  {
    DEBUG("SegmentBuffer:: First frame to display not ready yet");
    return {};
  }

  DEBUG("First frame to display ready.");
  this->eventCV.notify_all();
  return this->begin();
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToDisplay(FrameIterator frameIt)
{
  std::unique_lock<std::mutex> lk(this->segmentQueueMutex);

  if (this->aborted)
  {
    DEBUG("SegmentBuffer:: Next frame to display not ready because aborted");
    return {};
  }

  assert(!frameIt.isNull());

  auto nextFrame = frameIt++;
  if (nextFrame.isNull() || nextFrame->frameState != FrameState::ConvertedToRGB)
  {
    DEBUG("SegmentBuffer:: Next frame to display not ready yet");
    return {};
  }

  DEBUG("Next frame to display ready.");
  this->eventCV.notify_all();
  return nextFrame;
}