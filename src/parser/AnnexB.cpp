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

#include "AnnexB.h"

#include "parser/common/SubByteReaderLogging.h"

#include <QElapsedTimer>
#include <QProgressDialog>
#include <assert.h>

#define PARSERANNEXB_DEBUG_OUTPUT 0
#if PARSERANNEXB_DEBUG_OUTPUT && !NDEBUG
#include <QDebug>
#define DEBUG_ANNEXB(msg) qDebug() << msg
#else
#define DEBUG_ANNEXB(msg) ((void)0)
#endif

namespace parser
{

bool AnnexB::addFrameToList(int                       poc,
                            std::optional<pairUint64> fileStartEndPos,
                            bool                      randomAccessPoint)
{
  for (const auto &f : this->frameListCodingOrder)
    if (f.poc == poc)
      return false;

  if (pocOfFirstRandomAccessFrame == -1 && randomAccessPoint)
    pocOfFirstRandomAccessFrame = poc;
  if (poc >= pocOfFirstRandomAccessFrame)
  {
    // We don't add frames which we can not decode because they are before the first RA (I) frame
    AnnexBFrame newFrame;
    newFrame.poc               = poc;
    newFrame.fileStartEndPos   = fileStartEndPos;
    newFrame.randomAccessPoint = randomAccessPoint;
    this->frameListCodingOrder.push_back(newFrame);
    this->frameListDisplayOder.clear();
  }
  return true;
}

void AnnexB::logNALSize(const ByteVector &        data,
                        std::shared_ptr<TreeItem> root,
                        std::optional<pairUint64> nalStartEndPos)
{
  size_t startCodeSize = 0;
  if (data[0] == char(0) && data[1] == char(0) && data[2] == char(0) && data[3] == char(1))
    startCodeSize = 4;
  if (data[0] == char(0) && data[1] == char(0) && data[2] == char(1))
    startCodeSize = 3;

  if (startCodeSize > 0)
    root->createChildItem("Start code size", startCodeSize);

  root->createChildItem("Payload size", data.size() - startCodeSize);
  if (nalStartEndPos)
    root->createChildItem("Start/End pos", to_string(*nalStartEndPos));
}

auto AnnexB::getClosestSeekPoint(FrameIndexDisplayOrder targetFrame,
                                 FrameIndexDisplayOrder currentFrame) -> SeekPointInfo
{
  if (targetFrame >= this->frameListCodingOrder.size())
    return {};

  this->updateFrameListDisplayOrder();
  auto frameTarget  = this->frameListDisplayOder[targetFrame];
  auto frameCurrent = this->frameListDisplayOder[currentFrame];

  auto bestSeekFrame = this->frameListCodingOrder.begin();
  for (auto it = this->frameListCodingOrder.begin(); it != this->frameListCodingOrder.end(); it++)
  {
    if (it->randomAccessPoint && it->poc < frameTarget.poc)
      bestSeekFrame = it;
    if (it->poc == frameTarget.poc)
      break;
  }

  SeekPointInfo seekPointInfo;
  {
    auto itInDisplayOrder = std::find(
        this->frameListDisplayOder.begin(), this->frameListDisplayOder.end(), *bestSeekFrame);
    seekPointInfo.frameIndex = std::distance(this->frameListDisplayOder.begin(), itInDisplayOrder);
  }
  {
    auto itCurrentFrameCodingOrder = std::find(
        this->frameListCodingOrder.begin(), this->frameListCodingOrder.end(), frameCurrent);
    seekPointInfo.frameDistanceInCodingOrder =
        std::distance(itCurrentFrameCodingOrder, bestSeekFrame);
  }

  DEBUG_ANNEXB("AnnexB::getClosestSeekPoint targetFrame "
               << targetFrame << "(POC " << frameTarget.poc << " seek to "
               << seekPointInfo.frameIndex << " (POC " << bestSeekFrame->poc
               << ") distance in coding order " << seekPointInfo.frameDistanceInCodingOrder);
  return seekPointInfo;
}

std::optional<pairUint64> AnnexB::getFrameStartEndPos(FrameIndexCodingOrder idx)
{
  if (idx >= this->frameListCodingOrder.size())
    return {};
  this->updateFrameListDisplayOrder();
  return this->frameListCodingOrder[idx].fileStartEndPos;
}

QList<QTreeWidgetItem *> AnnexB::stream_info_type::getStreamInfo()
{
  QList<QTreeWidgetItem *> infoList;
  infoList.append(new QTreeWidgetItem(QStringList() << "File size" << QString::number(file_size)));
  if (parsing)
  {
    infoList.append(new QTreeWidgetItem(QStringList() << "Number NAL units"
                                                      << "Parsing..."));
    infoList.append(new QTreeWidgetItem(QStringList() << "Number Frames"
                                                      << "Parsing..."));
  }
  else
  {
    infoList.append(
        new QTreeWidgetItem(QStringList() << "Number NAL units" << QString::number(nr_nal_units)));
    infoList.append(
        new QTreeWidgetItem(QStringList() << "Number Frames" << QString::number(nr_frames)));
  }

  return infoList;
}

int AnnexB::getFramePOC(FrameIndexDisplayOrder frameIdx)
{
  this->updateFrameListDisplayOrder();
  return this->frameListDisplayOder[frameIdx].poc;
}

void AnnexB::updateFrameListDisplayOrder()
{
  if (this->frameListCodingOrder.size() == 0 || this->frameListDisplayOder.size() > 0)
    return;

  this->frameListDisplayOder = this->frameListCodingOrder;
  std::sort(frameListDisplayOder.begin(), frameListDisplayOder.end());
}

} // namespace parser
