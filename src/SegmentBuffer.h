/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <common/Segment.h>

#include <condition_variable>
#include <deque>
#include <iterator>
#include <shared_mutex>
#include <QObject>

/* The central storage for segments, frames and all their buffers
 *
 * The downloader pushes its downloaded Segments in here, next the decoder takes
 * a segment out and decodes it into a range of frames and lastly, the conversion thread
 * converts each frame from YUV to RGB. Finally, the player takes out the frames and displays
 * them. Each of the threads (download, decode, convert) may be blocked here if certain limits
 * are reached.
 * Segments are automatically removed from the list once all frames were displayed.
 */
class SegmentBuffer : public QObject
{
  Q_OBJECT

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

  using FramePt      = std::shared_ptr<Frame>;
  using SegmentPtr   = std::shared_ptr<Segment>;
  using SegmentDeque = std::deque<SegmentPtr>;

  struct FrameIterator
  {
    FrameIterator() = default;
    FrameIterator(SegmentPtr segment, FramePt frame) : segment(segment), frame(frame) {}
    bool       isNull() { return this->segment == nullptr || this->frame == nullptr; }
    SegmentPtr segment;
    FramePt    frame;
  };

  struct SegmentRenderInfo
  {
    double                  downloadProgress{};
    unsigned                sizeInBytes{};
    unsigned                nrFrames{};
    std::vector<FrameState> frameStates;
    std::optional<unsigned> indexOfCurFrameInFrames;
  };
  std::vector<SegmentRenderInfo> getBufferStatusForRender(FramePt curPlaybackFrame);

  // The downloader will get new segments from here. This will not block.
  SegmentPtr getNextDownloadSegment();

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

signals:
  void startNextDownload();

public slots:
  void onDownloadFinished();

private:
  void tryToStartNextDownload();

  SegmentDeque segments;

  std::condition_variable_any eventCV;
  std::shared_mutex           segmentQueueMutex;

  bool aborted{false};
};
