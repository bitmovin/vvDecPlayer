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
