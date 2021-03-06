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

#pragma once

#include <common/Segment.h>

#include <QObject>
#include <condition_variable>
#include <deque>
#include <iterator>
#include <optional>
#include <queue>
#include <shared_mutex>

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

  struct FrameIterator
  {
    FrameIterator() = default;
    FrameIterator(Segment *segment, Frame *frame) : segment(segment), frame(frame) {}
    bool     isNull() { return this->segment == nullptr || this->frame == nullptr; }
    Segment *segment{};
    Frame *  frame{};
  };

  struct SegmentRenderInfo
  {
    double                  downloadProgress{};
    std::size_t             sizeInBytes{};
    unsigned                nrFrames{};
    std::optional<unsigned> indexOfCurFrameInFrames;
    unsigned                segmentNumber{};
    unsigned                renditionNumber{};

    struct FrameInfo
    {
      FrameState  frameState{};
      std::size_t sizeInBytes{};
    };
    std::vector<FrameInfo> frameInfo;
  };
  std::vector<SegmentRenderInfo> getBufferStatusForRender(Frame *curPlaybackFrame);

  size_t getNrOfBufferedSegments();

  // These provide new (or maybe recycled) segments/frames. These do not block.
  Segment *getNextDownloadSegment();
  Frame *  addNewFrameToSegment(Segment *segment);

  // The parser will get segments to parser here (and may get blocked if no segment is ready yet)
  Segment *getFirstSegmentToParse();
  Segment *getNextSegmentToParse(Segment *segment);

  // The decoder will get segments to decode here (and may get blocked if too many
  // decoded frames are already in the buffer)
  Segment *getFirstSegmentToDecode();
  Segment *getNextSegmentToDecode(Segment *segment);
  void     onFrameDecoded();

  // The converter will get frames to convert here (and may get blocked if there
  // are none)
  FrameIterator getFirstFrameToConvert();
  FrameIterator getNextFrameToConvert(FrameIterator frameIt);

  // The player will get frames to display here (and may get none (end) if there is none available)
  FrameIterator getFirstFrameToDisplay();
  FrameIterator getNextFrameToDisplay(FrameIterator frameIt);

  void onDownloadOfSegmentFinished();

signals:
  void segmentRemovedFromBuffer();

private:
  std::deque<std::unique_ptr<Segment>> segments;

  std::condition_variable_any eventCV;
  std::shared_mutex           segmentQueueMutex;

  bool aborted{false};

  void                                 recycleSegmentAndFrames(std::unique_ptr<Segment> &&segment);
  std::queue<std::unique_ptr<Segment>> segmentRecycleBin;
  std::queue<std::unique_ptr<Frame>>   frameRecycleBin;
};
