/*  This file is part of YUView - The YUV player with advanced analytics toolset
 *   <https://github.com/IENT/YUView>
 *   Copyright (C) 2015  Institut f�r Nachrichtentechnik, RWTH Aachen University, GERMANY
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the
 *   OpenSSL library under certain conditions as described in each
 *   individual source file, and distribute linked combinations including
 *   the two.
 *
 *   You must obey the GNU General Public License in all respects for all
 *   of the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the
 *   file(s), but you are not obligated to do so. If you do not wish to do
 *   so, delete this exception statement from your version. If you delete
 *   this exception statement from all source files in the program, then
 *   also delete it here.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
