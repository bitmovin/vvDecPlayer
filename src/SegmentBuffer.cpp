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
    SegmentRenderInfo segmentInfo;
    segmentInfo.downloadProgress = segment->downloadProgress;
    segmentInfo.sizeInBytes      = segment->compressedSizeBytes;
    segmentInfo.nrFrames         = segment->nrFrames;
    segmentInfo.segmentNumber    = segment->playbackInfo.segmentNumber;
    segmentInfo.renditionNumber  = segment->playbackInfo.rendition;
    unsigned frameCounter        = 0;
    for (auto &frame : segment->frames)
    {
      SegmentRenderInfo::FrameInfo frameInfo;
      frameInfo.frameState  = frame->frameState;
      frameInfo.sizeInBytes = frame->nrBytesCompressed;
      segmentInfo.frameInfo.push_back(frameInfo);
      if (frame == curPlaybackFrame)
        segmentInfo.indexOfCurFrameInFrames = frameCounter;
      frameCounter++;
    }
    states.push_back(segmentInfo);
  }
  return states;
}

SegmentBuffer::SegmentPtr SegmentBuffer::getNextDownloadSegment()
{
  DEBUG("SegmentBuffer: Get next segment");

  if (this->aborted)
  {
    DEBUG("SegmentBuffer: Not returning next download segment because of abort");
    return {};
  }

  SegmentPtr newSegment;
  if (this->segmentRecycleBin.empty())
    newSegment = std::make_shared<Segment>();
  else
  {
    newSegment = this->segmentRecycleBin.front();
    this->segmentRecycleBin.pop();
  }

  this->segments.push_back(newSegment);
  return newSegment;
}

SegmentBuffer::FramePt SegmentBuffer::addNewFrameToSegment(SegmentPtr segment)
{
  std::shared_lock lk(this->segmentQueueMutex);

  FramePt newFrame;
  if (this->frameRecycleBin.empty())
    newFrame = std::make_shared<Frame>();
  else
  {
    newFrame = this->frameRecycleBin.front();
    this->frameRecycleBin.pop();
  }

  segment->frames.push_back(newFrame);
  segment->nrFrames++;
  return newFrame;
}

void SegmentBuffer::onDownloadFinished()
{
  this->eventCV.notify_all();
  this->tryToStartNextDownload();
}

SegmentBuffer::SegmentPtr SegmentBuffer::getFirstSegmentToParse()
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
  return *this->segments.begin();
}

SegmentBuffer::SegmentPtr SegmentBuffer::getNextSegmentToParse(SegmentPtr segmentPtr)
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

SegmentBuffer::SegmentPtr SegmentBuffer::getFirstSegmentToDecode()
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
  return {firstSegment, firstSegment->frames.front()};
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
    assert(frameIt.segment == this->segments.front());
    this->segments.pop_front();
    this->recycleSegmentAndFrames(frameIt.segment);
    this->tryToStartNextDownload();
  }

  DEBUG("Next frame to display ready.");
  this->eventCV.notify_all();
  return nextFrame;
}

void SegmentBuffer::tryToStartNextDownload()
{
  constexpr auto MAX_DOWNLOADED_SEGMENTS_IN_QUEUE = 5;
  if (this->segments.size() < MAX_DOWNLOADED_SEGMENTS_IN_QUEUE)
    emit startNextDownload();
}

void SegmentBuffer::recycleSegmentAndFrames(SegmentPtr segment)
{
  for (auto frameIt : segment->frames)
  {
    frameIt->clear();
    this->frameRecycleBin.push(frameIt);
  }
  segment->clear();
  this->segmentRecycleBin.push(segment);
}
