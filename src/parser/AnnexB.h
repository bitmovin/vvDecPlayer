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

#include <QList>
#include <QTreeWidgetItem>
#include <optional>
#include <set>

#include "common/TreeItem.h"
#include "YUV/YUVPixelFormat.h"

using namespace YUV_Internals;

namespace parser
{

struct BitrateEntry
{
  int     dts{0};
  int     pts{0};
  int     duration{1};
  size_t  bitrate{0};
  bool    keyframe{false};
  QString frameType{};
};

using FrameIndexDisplayOrder = unsigned;
using FrameIndexCodingOrder  = unsigned;

/* The (abstract) base class for the various types of AnnexB files (AVC, HEVC, VVC) that we can
 * parse.
 */
class AnnexB
{
public:
  AnnexB() = default;
  virtual ~AnnexB(){};

  // How many POC's have been found in the file
  size_t getNumberPOCs() const { return this->frameListCodingOrder.size(); }

  // Clear all knowledge about the bitstream.
  void clearData();

  /* Parse the NAL unit and what it contains
   *
   * It also adds the unit to the nalUnitList (if it is a parameter set or an RA point).
   * When there are no more NAL units in the file (the file ends), call this function one last time
   * with empty data and a nalID of -1. \nalID A counter (ID) of the nal \data The raw data of the
   * NAL. May include the start code or not. \bitrateEntry Pass the bitrate entry data into the
   * function that may already be known. E.g. the ffmpeg parser already decodes the DTS/PTS values
   * from the container. \parent The tree item of the parent where the items will be appended.
   * \nalStartEndPosFile The position of the first and last byte of the NAL.
   */
  struct ParseResult
  {
    ParseResult() = default;
    bool                        success{false};
    std::optional<std::string>  nalTypeName;
    std::optional<BitrateEntry> bitrateEntry;
  };
  virtual ParseResult parseAndAddNALUnit(int                         nalID,
                                         const ByteVector &          data,
                                         std::optional<BitrateEntry> bitrateEntry,
                                         std::optional<pairUint64>   nalStartEndPosFile = {}) = 0;

  // Get some format properties
  virtual double         getFramerate() const           = 0;
  virtual Size           getSequenceSizeSamples() const = 0;
  virtual YUVPixelFormat getPixelFormat() const         = 0;

  // When we want to seek to a specific frame number, this function return the parameter sets that
  // you need to start decoding (without start codes). If file positions were set for the NAL units,
  // the file position where decoding can begin will also be returned.
  struct SeekData
  {
    std::vector<ByteVector> parameterSets;
    std::optional<uint64_t> filePos;
  };
  virtual std::optional<SeekData> getSeekData(int iFrameNr) = 0;

  // Look through the random access points and find the closest one before (or equal)
  // the given frameIdx where we can start decoding
  // frameIdx: The frame index in display order that we want to seek to
  struct SeekPointInfo
  {
    FrameIndexDisplayOrder frameIndex{};
    unsigned               frameDistanceInCodingOrder{};
  };
  auto getClosestSeekPoint(FrameIndexDisplayOrder targetFrame, FrameIndexDisplayOrder currentFrame)
      -> SeekPointInfo;

  // Get the parameters sets as extradata. The format of this depends on the underlying codec.
  virtual QByteArray getExtradata() = 0;
  // Get some other properties of the bitstream in order to configure the FFMpegDecoder
  virtual IntPair getProfileLevel()      = 0;
  virtual Ratio   getSampleAspectRatio() = 0;

  std::optional<pairUint64> getFrameStartEndPos(FrameIndexCodingOrder idx);

protected:
  struct AnnexBFrame
  {
    AnnexBFrame() = default;
    int poc{-1}; //< The poc of this frame
    std::optional<pairUint64>
         fileStartEndPos;          //< The start and end position of all slice NAL units (if known)
    bool randomAccessPoint{false}; //< Can we start decoding here?

    bool operator<(AnnexBFrame const &b) const { return (this->poc < b.poc); }
    bool operator==(AnnexBFrame const &b) const { return (this->poc == b.poc); }
  };

  // Returns false if the POC was already present int the list
  bool addFrameToList(int poc, std::optional<pairUint64> fileStartEndPos, bool randomAccessPoint);

  static void logNALSize(const ByteVector &        data,
                         std::shared_ptr<TreeItem> root,
                         std::optional<pairUint64> nalStartEndPos);

  int pocOfFirstRandomAccessFrame{-1};

  // Save general information about the file here
  struct stream_info_type
  {
    QList<QTreeWidgetItem *> getStreamInfo();

    size_t   file_size;
    unsigned nr_nal_units{0};
    unsigned nr_frames{0};
    bool     parsing{false};
  };
  stream_info_type stream_info;

  int getFramePOC(FrameIndexDisplayOrder frameIdx);

private:
  // A list of all frames in the sequence (in coding order) with POC and the file positions of all
  // slice NAL units associated with a frame. POC's don't have to be consecutive, so the only way to
  // know how many pictures are in a sequences is to keep a list of all POCs.
  std::vector<AnnexBFrame> frameListCodingOrder;
  // The same list of frames but sorted in display order. Generated from the list above whenever
  // needed.
  std::vector<AnnexBFrame> frameListDisplayOder;
  void                updateFrameListDisplayOrder();
};

} // namespace parser