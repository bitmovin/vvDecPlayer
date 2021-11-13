/* MIT License

Copyright (c) 2021 Christian Feldmann <christian.feldmann@gmx.de>
                                      <christian.feldmann@bitmovin.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "SegmentBuffer.h"

#include <assert.h>

#define DEBUG_SEGMENT_BUFFER 0
#if DEBUG_SEGMENT_BUFFER
#include <QDebug>
#define DEBUG(f) qDebug() << f
#else
#define DEBUG(f) ((void)0)
#endif

namespace
{

Segment *getNextSegmentFromQueue(Segment *                             curSegment,
                                 std::deque<std::unique_ptr<Segment>> &segments)
{
  auto segmentIt = std::find_if(
      segments.begin(), segments.end(), [&curSegment](const std::unique_ptr<Segment> &segment) {
        return segment.get() == curSegment;
      });
  if (segmentIt == segments.end())
    return {};
  segmentIt++;
  if (segmentIt == segments.end())
    return {};
  return segmentIt->get();
}

SegmentBuffer::FrameIterator getNextFrame(const SegmentBuffer::FrameIterator &  frameIterator,
                                          std::deque<std::unique_ptr<Segment>> &segments)
{
  auto segmentIt = std::find_if(
      segments.begin(), segments.end(), [&frameIterator](const std::unique_ptr<Segment> &segment) {
        return segment.get() == frameIterator.segment;
      });
  if (segmentIt == segments.end())
    return {};
  auto &segmentFrames  = (*segmentIt)->frames;
  auto  segmentFrameIt = std::find_if(segmentFrames.begin(),
                                     segmentFrames.end(),
                                     [&frameIterator](const std::unique_ptr<Frame> &frame) {
                                       return frame.get() == frameIterator.frame;
                                     });
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
    return {nextSegment->get(), (*nextSegment)->frames.front().get()};
  }
  return {frameIterator.segment, nextFrameInSegment->get()};
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
SegmentBuffer::getBufferStatusForRender(Frame *curPlaybackFrame)
{
  std::unique_lock               lk(this->segmentQueueMutex);
  std::vector<SegmentRenderInfo> states;
  for (auto &segment : this->segments)
  {
    SegmentRenderInfo segmentInfo;
    segmentInfo.downloadProgress = segment->downloadProgress;
    segmentInfo.sizeInBytes      = segment->compressedSizeBytes;
    segmentInfo.nrFrames         = segment->nrFrames;
    segmentInfo.segmentNumber    = segment->segmentInfo.segmentNumber;
    segmentInfo.renditionNumber  = segment->segmentInfo.rendition;
    unsigned frameCounter        = 0;
    for (auto &frame : segment->frames)
    {
      SegmentRenderInfo::FrameInfo frameInfo;
      frameInfo.frameState  = frame->frameState;
      frameInfo.sizeInBytes = frame->nrBytesCompressed;
      segmentInfo.frameInfo.push_back(frameInfo);
      if (frame.get() == curPlaybackFrame)
        segmentInfo.indexOfCurFrameInFrames = frameCounter;
      frameCounter++;
    }
    states.push_back(segmentInfo);
  }
  return states;
}

size_t SegmentBuffer::getNrOfBufferedSegments() { return this->segments.size(); }

Segment *SegmentBuffer::getNextDownloadSegment()
{
  DEBUG("SegmentBuffer: Get next segment");

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Not returning next download segment because of abort");
    return {};
  }

  if (this->segmentRecycleBin.empty())
    this->segments.emplace_back(std::make_unique<Segment>());
  else
  {
    this->segments.push_back(std::move(this->segmentRecycleBin.front()));
    this->segmentRecycleBin.pop();
  }

  return this->segments.back().get();
}

Frame *SegmentBuffer::addNewFrameToSegment(Segment *segment)
{
  std::shared_lock lk(this->segmentQueueMutex);

  if (this->frameRecycleBin.empty())
    segment->frames.emplace_back(std::make_unique<Frame>());
  else
  {
    segment->frames.push_back(std::move(this->frameRecycleBin.front()));
    this->frameRecycleBin.pop();
  }

  segment->nrFrames++;
  return segment->frames.back().get();
}

void SegmentBuffer::onDownloadOfSegmentFinished() { this->eventCV.notify_all(); }

Segment *SegmentBuffer::getFirstSegmentToParse()
{
  DEBUG("SegmentBuffer: Waiting for first segment to parse.");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    if (this->aborted)
      return true;
    if (this->segments.size() == 0)
      return false;
    return (*this->segments.begin())->downloadFinished;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First segment to parse not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: First segment to parse ready");
  return this->segments.begin()->get();
}

Segment *SegmentBuffer::getNextSegmentToParse(Segment *segmentPtr)
{
  DEBUG("SegmentBuffer: Waiting for next segment to parse");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentPtr]() {
    if (this->aborted)
      return true;
    if (auto nextSegment = getNextSegmentFromQueue(segmentPtr, this->segments))
      return nextSegment->downloadFinished;
    return false;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next segment to parse not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: Next segment to parse ready");
  return getNextSegmentFromQueue(segmentPtr, this->segments);
}

Segment *SegmentBuffer::getFirstSegmentToDecode()
{
  DEBUG("SegmentBuffer: Waiting for first segment to decode.");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this]() {
    if (this->aborted)
      return true;
    if (this->segments.size() == 0)
      return false;
    return (*this->segments.begin())->parsingFinished;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First segment to decode not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: First segment to decode ready");
  return this->segments.begin()->get();
}

Segment *SegmentBuffer::getNextSegmentToDecode(Segment *segmentPtr)
{
  DEBUG("SegmentBuffer: Waiting for next segment to decode");
  this->eventCV.notify_all();

  std::shared_lock lk(this->segmentQueueMutex);
  this->eventCV.wait(lk, [this, segmentPtr]() {
    if (this->aborted)
      return true;
    if (auto nextSegment = getNextSegmentFromQueue(segmentPtr, this->segments))
      return nextSegment->parsingFinished;
    return false;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Next segment to decode not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: Next segment to decode ready");
  return getNextSegmentFromQueue(segmentPtr, this->segments);
}

void SegmentBuffer::onFrameDecoded()
{
  // Whenever a frame was decoded we can already convert it
  this->eventCV.notify_all();
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
    if (!frameAvailabe)
      return false;
    return this->segments.front()->frames.at(0)->frameState == FrameState::Decoded;
  });

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: First frame to convert not ready because of abort");
    return {};
  }

  DEBUG("SegmentBuffer: First frame to convert ready");
  return {this->segments.front().get(), this->segments.front()->frames.front().get()};
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
    if (nextFrame.isNull())
      return false;
    return nextFrame.frame->frameState == FrameState::Decoded;
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
  return {firstSegment.get(), firstSegment->frames.front().get()};
}

SegmentBuffer::FrameIterator SegmentBuffer::getNextFrameToDisplay(FrameIterator frameIt)
{
  assert(!frameIt.isNull());
  std::shared_lock             lk(this->segmentQueueMutex);
  SegmentBuffer::FrameIterator nextFrame;

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

  if (frameIt.segment != nextFrame.segment)
  {
    assert(frameIt.segment == this->segments.front().get());
    this->recycleSegmentAndFrames(std::move(this->segments.front()));
    this->segments.pop_front();
    emit segmentRemovedFromBuffer();
  }

  DEBUG("Next frame to display ready.");
  this->eventCV.notify_all();
  return nextFrame;
}

void SegmentBuffer::recycleSegmentAndFrames(std::unique_ptr<Segment> &&segment)
{
  for (auto &frameIt : segment->frames)
  {
    frameIt->clear();
    this->frameRecycleBin.push(std::move(frameIt));
  }
  segment->clear();
  this->segmentRecycleBin.push(std::move(segment));
}
