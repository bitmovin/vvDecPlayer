/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <common/Segment.h>

#include <condition_variable>
#include <deque>
#include <iterator>
#include <mutex>

/* The central storage for segments, frames and all their buffers
 *
 * The downloader pushes its downloaded Segments in here, next the decoder takes
 * a segment out and decodes it into a range of frames and lastly, the conversion thread
 * converts each frame from YUV to RGB. Finally, the player takes out the frames and displays
 * them. Each of the threads (download, decode, convert) may be blocked here if certain limits
 * are reached.
 * Segments are automatically removed from the list once all frames were displayed.
 */
class SegmentBuffer
{
public:
  SegmentBuffer()  = default;
  ~SegmentBuffer();

  void abort();

  using SegmentIt = std::deque<Segment>::iterator;
  using FrameIt   = std::vector<Frame>::iterator;

  struct FrameIterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = Frame;
    using pointer           = Frame *;
    using reference         = Frame &;

    FrameIterator() = default;

    FrameIterator(const FrameIterator &it)
        : segmentIt(it.segmentIt), frameIt(it.frameIt), segments(it.segments)
    {
    }

    FrameIterator(SegmentIt segmentIt, FrameIt frameIt, std::deque<Segment> *segments)
        : segmentIt(segmentIt), frameIt(frameIt), segments(segments)
    {
    }

    Segment *getSegment() { return &(*this->segmentIt); }

    reference      operator*() const { return *this->frameIt; }
    pointer        operator->() { return &(*this->frameIt); }
    FrameIterator &operator++()
    {
      this->frameIt++;
      if (this->frameIt == this->segmentIt->frames.end())
      {
        this->segmentIt++;
        if (this->segmentIt != this->segments->end())
        {
          if (this->segmentIt->frames.size() == 0)
          {
            // The next segment has no frames. In this case we also return end().
            this->segmentIt = this->segments->end();
            return *this;
          }
          this->frameIt = this->segmentIt->frames.begin();
        }
      }
      return *this;
    }
    FrameIterator operator++(int)
    {
      FrameIterator tmp = *this;
      ++(*this);
      return tmp;
    }
    friend bool operator==(const FrameIterator &a, const FrameIterator &b)
    {
      return a.segmentIt == b.segmentIt && a.frameIt == b.frameIt;
    };
    friend bool operator!=(const FrameIterator &a, const FrameIterator &b)
    {
      return a.segmentIt != b.segmentIt || a.frameIt != b.frameIt;
    };

  private:
    std::deque<Segment>::iterator segmentIt;
    std::vector<Frame>::iterator  frameIt;
    std::deque<Segment> *         segments{};
  };

  FrameIterator begin();
  FrameIterator end();

  SegmentIt beginSegment() { return this->segments.begin(); }
  SegmentIt endSegment() { return this->segments.end(); }

  // The downloader will push downloaded segments in here (and may get blocked if the buffer is
  // full)
  void pushDownloadedSegment(Segment segment);

  // The decoder will get segments to decode here (and may get blocked if too many
  // decoded frames are already in the buffer)
  SegmentIt getFirstSegmentToDecode();
  SegmentIt getNextSegmentToDecode(SegmentIt segmentIt);

  // The converter will get frames to convert here (and may get blocked if there
  // are none)
  FrameIterator getFirstFrameToConvert();
  FrameIterator getNextFrameToConvert(FrameIterator frameIt);

  // The player will get frames to display here (and may get none (end) if there is none available)
  FrameIterator getFirstFrameToDisplay();
  FrameIterator getNextFrameToDisplay(FrameIterator frameIt);

private:
  std::deque<Segment> segments;

  std::condition_variable eventCV;
  std::mutex              segmentQueueMutex;

  bool aborted{false};
};
