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
  SegmentBuffer() = default;
  ~SegmentBuffer();

  void abort();

  /* The deque idea does not work. When adding things to the queue all iterators are invalidated.
   * New idea:
   *   - We store shared pointers to Segments in the queue. For going to the next segment we then
   *     have to search though the queue but it only has a few items.
   *   - The FrameIterator can use the current interace using also shared pointers to the segment
   */

  using FrameIt      = std::vector<Frame>::iterator;
  using SegmentPtr   = std::shared_ptr<Segment>;
  using SegmentDeque = std::deque<SegmentPtr>;

  struct FrameIterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = Frame;
    using pointer           = Frame *;
    using reference         = Frame &;

    FrameIterator() = default;
    FrameIterator(const FrameIterator &it);
    FrameIterator(SegmentPtr segment, FrameIt frameIt, SegmentDeque *segments);

    SegmentPtr getSegment() { return this->curSegment; }
    bool       isNull() { return this->segments == nullptr; }

    reference      operator*() const { return *this->frameIt; }
    pointer        operator->() { return &(*this->frameIt); }
    FrameIterator &operator++();
    FrameIterator operator++(int);
    friend bool operator==(const FrameIterator &a, const FrameIterator &b)
    {
      if (a.segments == nullptr)
        return b.segments == nullptr;
      return a.curSegment == b.curSegment && a.frameIt == b.frameIt;
    };
    friend bool operator!=(const FrameIterator &a, const FrameIterator &b)
    {
      if (a.segments == nullptr)
        return b.segments != nullptr;
      return a.curSegment != b.curSegment || a.frameIt != b.frameIt;
    };

  private:
    SegmentPtr                   curSegment;
    std::vector<Frame>::iterator frameIt;
    SegmentDeque *               segments{};
  };

  FrameIterator begin();
  FrameIterator end();

  // The downloader will push downloaded segments in here (and may get blocked if the buffer is
  // full)
  void pushDownloadedSegment(SegmentPtr segment);

  // The decoder will get segments to decode here (and may get blocked if too many
  // decoded frames are already in the buffer)
  SegmentPtr getFirstSegmentToDecode();
  SegmentPtr getNextSegmentToDecode(SegmentPtr segment);

  // The converter will get frames to convert here (and may get blocked if there
  // are none)
  FrameIterator getFirstFrameToConvert();
  FrameIterator getNextFrameToConvert(FrameIterator frameIt);

  // The player will get frames to display here (and may get none (end) if there is none available)
  FrameIterator getFirstFrameToDisplay();
  FrameIterator getNextFrameToDisplay(FrameIterator frameIt);

private:
  SegmentDeque segments;

  std::condition_variable eventCV;
  std::mutex              segmentQueueMutex;

  bool aborted{false};
};
