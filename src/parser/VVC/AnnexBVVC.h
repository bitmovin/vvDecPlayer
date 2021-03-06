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

#include "../AnnexB.h"
#include "NalUnitVVC.h"
#include "commonMaps.h"
#include <video/PixelFormatYUV.h>


#include <memory>

namespace parser
{

namespace vvc
{

class slice_layer_rbsp;
class picture_header_structure;
class buffering_period;

struct ParsingState
{
  using sharedPictureHeader = std::shared_ptr<vvc::picture_header_structure>;
  sharedPictureHeader currentPictureHeaderStructure;
  using Nuh_Layer_Id = unsigned;
  std::map<Nuh_Layer_Id, sharedPictureHeader> prevTid0Pic;

  std::shared_ptr<vvc::slice_layer_rbsp> currentSlice;
  std::shared_ptr<vvc::buffering_period> lastBufferingPeriod;

  struct CurrentAU
  {
    size_t                    counter{};
    size_t                    sizeBytes{};
    int                       poc{-1};
    bool                      isKeyframe{};
    std::optional<pairUint64> fileStartEndPos;
  };
  CurrentAU currentAU{};

  using LayerID = unsigned;
  std::map<LayerID, bool> NoOutputBeforeRecoveryFlag;
};

} // namespace vvc

// This class knows how to parse the bitrstream of VVC annexB files
class AnnexBVVC : public AnnexB
{
public:
  AnnexBVVC() : AnnexB(){};
  ~AnnexBVVC() = default;

  // Get some properties
  Size                       getSequenceSizeSamples() const override;
  video::yuv::PixelFormatYUV getPixelFormat() const override;

  virtual std::optional<SeekData> getSeekData(int iFrameNr) override;
  QByteArray                      getExtradata() override;
  IntPair                         getProfileLevel() override;
  Ratio                           getSampleAspectRatio() override;

  ParseResult parseAndAddNALUnit(int                         nalID,
                                 const ByteVector &          data,
                                 std::optional<BitrateEntry> bitrateEntry,
                                 std::optional<pairUint64>   nalStartEndPosFile = {}) override;

protected:
  // The PicOrderCntMsb may be reset to zero for IDR frames. In order to count the global POC, we
  // store the maximum POC.
  uint64_t maxPOCCount{0};
  uint64_t pocCounterOffset{0};
  int      calculateAndUpdateGlobalPOC(bool isIRAP, unsigned PicOrderCntVal);

  struct ActiveParameterSets
  {
    vvc::VPSMap vpsMap;
    vvc::SPSMap spsMap;
    vvc::PPSMap ppsMap;
    vvc::APSMap apsMap;
  };
  ActiveParameterSets activeParameterSets;

  std::vector<std::shared_ptr<vvc::NalUnitVVC>> nalUnitsForSeeking;

  vvc::ParsingState parsingState;
  bool              handleNewAU(vvc::ParsingState &updatedParsingState);

  struct auDelimiterDetector_t
  {
    bool     isStartOfNewAU(std::shared_ptr<vvc::NalUnitVVC>               nal,
                            std::shared_ptr<vvc::picture_header_structure> ph);
    bool     lastNalWasVcl{false};
    unsigned lastVcl_PicOrderCntVal{};
    unsigned lastVcl_ph_pic_order_cnt_lsb{};
    unsigned lastVcl_nuh_layer_id;
  };
  auDelimiterDetector_t auDelimiterDetector;
};

} // namespace parser