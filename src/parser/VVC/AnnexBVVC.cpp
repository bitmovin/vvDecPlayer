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
#include "AnnexBVVC.h"

#include <algorithm>
#include <cmath>

#include "adaptation_parameter_set_rbsp.h"
#include "decoding_capability_information_rbsp.h"
#include "nal_unit_header.h"
#include "operating_point_information_rbsp.h"
#include "pic_parameter_set_rbsp.h"
#include "picture_header_rbsp.h"
#include "seq_parameter_set_rbsp.h"
#include "slice_layer_rbsp.h"
#include "video_parameter_set_rbsp.h"

#define PARSER_VVC_DEBUG_OUTPUT 0
#if PARSER_VVC_DEBUG_OUTPUT && !NDEBUG
#include <QDebug>
#define DEBUG_VVC(msg) qDebug() << msg
#else
#define DEBUG_VVC(msg) ((void)0)
#endif

namespace parser
{

using namespace vvc;

double AnnexBVVC::getFramerate() const
{
  // Parsing of VUI not implemented yet
  return DEFAULT_FRAMERATE;
}

Size AnnexBVVC::getSequenceSizeSamples() const
{
  for (const auto &nal : this->nalUnitsForSeeking)
  {
    if (nal->header.nal_unit_type == vvc::NalType::SPS_NUT)
    {
      auto sps = std::dynamic_pointer_cast<seq_parameter_set_rbsp>(nal->rbsp);
      return Size(sps->get_max_width_cropping(), sps->get_max_height_cropping());
    }
  }

  return {};
}

YUVPixelFormat AnnexBVVC::getPixelFormat() const
{
  // Get the subsampling and bit-depth from the sps
  int  bitDepthY   = -1;
  int  bitDepthC   = -1;
  auto subsampling = Subsampling::UNKNOWN;
  for (const auto &nal : this->nalUnitsForSeeking)
  {
    if (nal->header.nal_unit_type == vvc::NalType::SPS_NUT)
    {
      auto sps = std::dynamic_pointer_cast<seq_parameter_set_rbsp>(nal->rbsp);
      if (sps->sps_chroma_format_idc == 0)
        subsampling = Subsampling::YUV_400;
      else if (sps->sps_chroma_format_idc == 1)
        subsampling = Subsampling::YUV_420;
      else if (sps->sps_chroma_format_idc == 2)
        subsampling = Subsampling::YUV_422;
      else if (sps->sps_chroma_format_idc == 3)
        subsampling = Subsampling::YUV_444;

      bitDepthY = sps->sps_bitdepth_minus8 + 8;
      bitDepthC = sps->sps_bitdepth_minus8 + 8;
    }

    if (bitDepthY != -1 && bitDepthC != -1 && subsampling != Subsampling::UNKNOWN)
    {
      if (bitDepthY != bitDepthC)
      {
        // Different luma and chroma bit depths currently not supported
        return {};
      }
      return YUVPixelFormat(subsampling, bitDepthY);
    }
  }

  return {};
}

std::optional<AnnexB::SeekData> AnnexBVVC::getSeekData(int iFrameNr)
{
  if (iFrameNr >= int(this->getNumberPOCs()) || iFrameNr < 0)
    return {};

  auto seekPOC = this->getFramePOC(unsigned(iFrameNr));

  // Collect the active parameter sets
  using NalMap = std::map<unsigned, std::shared_ptr<NalUnitVVC>>;
  NalMap activeVPSNal;
  NalMap activeSPSNal;
  NalMap activePPSNal;
  NalMap activeAPSNal;

  for (const auto &nal : this->nalUnitsForSeeking)
  {
    if (nal->header.isSlice())
    {
      auto slice = std::dynamic_pointer_cast<slice_layer_rbsp>(nal->rbsp);

      auto picHeader = slice->slice_header_instance.picture_header_structure_instance;
      if (!picHeader)
      {
        DEBUG_VVC("Error - Slice has no picture header");
        continue;
      }

      if (seekPOC >= 0 && picHeader->PicOrderCntVal == unsigned(seekPOC))
      {
        // Seek here
        AnnexB::SeekData seekData;
        if (nal->filePosStartEnd)
          seekData.filePos = nal->filePosStartEnd->first;

        for (const auto &nalMap : {activeVPSNal, activeSPSNal, activePPSNal, activeAPSNal})
        {
          for (auto const &entry : nalMap)
            seekData.parameterSets.push_back(entry.second->rawData);
        }

        return seekData;
      }
    }
    if (nal->header.nal_unit_type == NalType::VPS_NUT)
    {
      auto vps = std::dynamic_pointer_cast<video_parameter_set_rbsp>(nal->rbsp);
      activeVPSNal[vps->vps_video_parameter_set_id] = nal;
    }
    if (nal->header.nal_unit_type == NalType::SPS_NUT)
    {
      auto sps = std::dynamic_pointer_cast<seq_parameter_set_rbsp>(nal->rbsp);
      activeSPSNal[sps->sps_seq_parameter_set_id] = nal;
    }
    if (nal->header.nal_unit_type == NalType::PPS_NUT)
    {
      auto pps = std::dynamic_pointer_cast<pic_parameter_set_rbsp>(nal->rbsp);
      activePPSNal[pps->pps_pic_parameter_set_id] = nal;
    }
    if (nal->header.nal_unit_type == NalType::PREFIX_APS_NUT ||
        nal->header.nal_unit_type == NalType::SUFFIX_APS_NUT)
    {
      auto aps = std::dynamic_pointer_cast<adaptation_parameter_set_rbsp>(nal->rbsp);
      activeAPSNal[aps->aps_adaptation_parameter_set_id] = nal;
    }
  }

  return {};
}

QByteArray AnnexBVVC::getExtradata() { return {}; }

IntPair AnnexBVVC::getProfileLevel()
{
  for (const auto &nal : this->nalUnitsForSeeking)
  {
    if (nal->header.nal_unit_type == vvc::NalType::SPS_NUT)
    {
      auto sps = std::dynamic_pointer_cast<seq_parameter_set_rbsp>(nal->rbsp);
      return {sps->profile_tier_level_instance.general_profile_idc,
              sps->profile_tier_level_instance.general_level_idc};
    }
  }
  return {};
}

Ratio AnnexBVVC::getSampleAspectRatio()
{
  for (const auto &nal : this->nalUnitsForSeeking)
  {
    if (nal->header.nal_unit_type == vvc::NalType::SPS_NUT)
    {
      auto sps = std::dynamic_pointer_cast<seq_parameter_set_rbsp>(nal->rbsp);
      if (sps->sps_vui_parameters_present_flag)
      {
        auto vui = sps->vui_payload_instance.vui;
        if (vui.vui_aspect_ratio_info_present_flag)
        {
          if (vui.vui_aspect_ratio_idc == 255)
            return {int(vui.vui_sar_height), int(vui.vui_sar_width)};
          else
            return sampleAspectRatioCoding.getValue(vui.vui_aspect_ratio_idc);
        }
      }
    }
  }
  return Ratio({1, 1});
}

AnnexB::ParseResult AnnexBVVC::parseAndAddNALUnit(int                         nalID,
                                                  const ByteVector &          data,
                                                  std::optional<BitrateEntry> bitrateEntry,
                                                  std::optional<pairUint64>   nalStartEndPosFile)
{
  AnnexB::ParseResult parseResult;
  parseResult.success = true;

  if (nalID == -1 && data.empty())
  {
    if (this->parsingState.lastFramePOC != -1)
    {
      if (!this->handleNewAU(this->parsingState, parseResult, bitrateEntry, nalStartEndPosFile))
      {
        DEBUG_VVC("Error handling last AU");
        parseResult.success = false;
      }
    }
    return parseResult;
  }

  // Skip the NAL unit header
  int readOffset = 0;
  if (data.at(0) == (char)0 && data.at(1) == (char)0 && data.at(2) == (char)1)
    readOffset = 3;
  else if (data.at(0) == (char)0 && data.at(1) == (char)0 && data.at(2) == (char)0 &&
           data.at(3) == (char)1)
    readOffset = 4;

  // Use the given tree item. If it is not set, use the nalUnitMode (if active).
  // Create a new TreeItem root for the NAL unit. We don't set data (a name) for this item
  // yet. We want to parse the item and then set a good description.
  std::shared_ptr<TreeItem> nalRoot;
  // No logging here
  // if (parent)
  //   nalRoot = parent->createChildItem();
  // else if (packetModel->rootItem)
  //   nalRoot = packetModel->rootItem->createChildItem();

  if (nalRoot)
    AnnexB::logNALSize(data, nalRoot, nalStartEndPosFile);

  reader::SubByteReaderLogging reader(data, nalRoot, "", readOffset);

  ParsingState updatedParsingState = this->parsingState;

  std::string specificDescription;
  auto        nalVVC = std::make_shared<vvc::NalUnitVVC>(nalID, nalStartEndPosFile);
  try
  {
    nalVVC->header.parse(reader);

    if (nalVVC->header.nal_unit_type == NalType::VPS_NUT)
    {
      specificDescription = " VPS";
      auto newVPS         = std::make_shared<video_parameter_set_rbsp>();
      newVPS->parse(reader);

      this->activeParameterSets.vpsMap[newVPS->vps_video_parameter_set_id] = newVPS;

      specificDescription += " ID " + std::to_string(newVPS->vps_video_parameter_set_id);

      nalVVC->rbsp    = newVPS;
      nalVVC->rawData = data;
      this->nalUnitsForSeeking.push_back(nalVVC);
    }
    else if (nalVVC->header.nal_unit_type == NalType::SPS_NUT)
    {
      specificDescription = " SPS";
      auto newSPS         = std::make_shared<seq_parameter_set_rbsp>();
      newSPS->parse(reader);

      this->activeParameterSets.spsMap[newSPS->sps_seq_parameter_set_id] = newSPS;

      specificDescription += " ID " + std::to_string(newSPS->sps_seq_parameter_set_id);

      nalVVC->rbsp    = newSPS;
      nalVVC->rawData = data;
      this->nalUnitsForSeeking.push_back(nalVVC);
    }
    else if (nalVVC->header.nal_unit_type == NalType::PPS_NUT)
    {
      specificDescription = " PPS";
      auto newPPS         = std::make_shared<pic_parameter_set_rbsp>();
      newPPS->parse(reader, this->activeParameterSets.spsMap);

      this->activeParameterSets.ppsMap[newPPS->pps_pic_parameter_set_id] = newPPS;

      specificDescription += " ID " + std::to_string(newPPS->pps_pic_parameter_set_id);

      nalVVC->rbsp    = newPPS;
      nalVVC->rawData = data;
      this->nalUnitsForSeeking.push_back(nalVVC);
    }
    else if (nalVVC->header.nal_unit_type == NalType::PREFIX_APS_NUT ||
             nalVVC->header.nal_unit_type == NalType::SUFFIX_APS_NUT)
    {
      specificDescription = " APS";
      auto newAPS         = std::make_shared<adaptation_parameter_set_rbsp>();
      newAPS->parse(reader);

      this->activeParameterSets.apsMap[newAPS->aps_adaptation_parameter_set_id] = newAPS;

      specificDescription += " ID " + std::to_string(newAPS->aps_adaptation_parameter_set_id);

      nalVVC->rbsp    = newAPS;
      nalVVC->rawData = data;
      this->nalUnitsForSeeking.push_back(nalVVC);
    }
    else if (nalVVC->header.nal_unit_type == NalType::PH_NUT)
    {
      specificDescription   = " Picture Header";
      auto newPictureHeader = std::make_shared<picture_header_rbsp>();
      newPictureHeader->parse(reader,
                              this->activeParameterSets.vpsMap,
                              this->activeParameterSets.spsMap,
                              this->activeParameterSets.ppsMap,
                              updatedParsingState.currentSlice);
      newPictureHeader->picture_header_structure_instance->calculatePictureOrderCount(
          reader,
          nalVVC->header.nal_unit_type,
          this->activeParameterSets.spsMap,
          this->activeParameterSets.ppsMap,
          updatedParsingState.currentPictureHeaderStructure);

      if (updatedParsingState.currentPictureHeaderStructure)
        updatedParsingState.lastFramePOC =
            updatedParsingState.currentPictureHeaderStructure->PicOrderCntVal;

      updatedParsingState.currentPictureHeaderStructure =
          newPictureHeader->picture_header_structure_instance;

      specificDescription +=
          " POC " +
          std::to_string(newPictureHeader->picture_header_structure_instance->PicOrderCntVal);

      nalVVC->rbsp = newPictureHeader;
    }
    else if (nalVVC->header.isSlice())
    {
      specificDescription = " Slice Header";
      auto newSliceLayer  = std::make_shared<slice_layer_rbsp>();
      newSliceLayer->parse(reader,
                           nalVVC->header.nal_unit_type,
                           this->activeParameterSets.vpsMap,
                           this->activeParameterSets.spsMap,
                           this->activeParameterSets.ppsMap,
                           updatedParsingState.currentPictureHeaderStructure);

      updatedParsingState.currentSlice = newSliceLayer;
      if (newSliceLayer->slice_header_instance.picture_header_structure_instance)
      {
        newSliceLayer->slice_header_instance.picture_header_structure_instance
            ->calculatePictureOrderCount(reader,
                                         nalVVC->header.nal_unit_type,
                                         this->activeParameterSets.spsMap,
                                         this->activeParameterSets.ppsMap,
                                         updatedParsingState.currentPictureHeaderStructure);
        updatedParsingState.currentPictureHeaderStructure =
            newSliceLayer->slice_header_instance.picture_header_structure_instance;
        updatedParsingState.lastFramePOC =
            updatedParsingState.currentPictureHeaderStructure->PicOrderCntVal;
      }
      else
      {
        if (!updatedParsingState.currentPictureHeaderStructure)
          throw std::logic_error("Slice must have a valid picture header");
        newSliceLayer->slice_header_instance.picture_header_structure_instance =
            updatedParsingState.currentPictureHeaderStructure;
      }

      specificDescription +=
          " POC " +
          std::to_string(updatedParsingState.currentPictureHeaderStructure->PicOrderCntVal);
      specificDescription +=
          " " + to_string(newSliceLayer->slice_header_instance.sh_slice_type) + "-Slice";

      nalVVC->rbsp = newSliceLayer;
    }
    else if (nalVVC->header.nal_unit_type == NalType::AUD_NUT)
    {
      specificDescription = " AUD";
    }
    else if (nalVVC->header.nal_unit_type == NalType::DCI_NUT)
    {
      specificDescription = " DCI";
      auto newDCI         = std::make_shared<decoding_capability_information_rbsp>();
      newDCI->parse(reader);
      nalVVC->rbsp = newDCI;
    }
    else if (nalVVC->header.nal_unit_type == NalType::EOB_NUT)
    {
      specificDescription = " EOB";
    }
    else if (nalVVC->header.nal_unit_type == NalType::EOS_NUT)
    {
      specificDescription = " EOS";
    }
    else if (nalVVC->header.nal_unit_type == NalType::FD_NUT)
    {
      specificDescription = " Filler Data";
    }
    else if (nalVVC->header.nal_unit_type == NalType::FD_NUT)
    {
      specificDescription = " OPI";
      auto newOPI         = std::make_shared<operating_point_information_rbsp>();
      newOPI->parse(reader);
      nalVVC->rbsp = newOPI;
    }
    else if (nalVVC->header.nal_unit_type == NalType::SUFFIX_SEI_NUT ||
             nalVVC->header.nal_unit_type == NalType::PREFIX_APS_NUT)
    {
      specificDescription = " SEI";
    }
  }
  catch (const std::exception &e)
  {
    specificDescription += " ERROR " + std::string(e.what());
    parseResult.success = false;
  }

  DEBUG_VVC("AnnexBVVC::parseAndAddNALUnit NAL " + QString::fromStdString(specificDescription));

  updatedParsingState.lastFrameIsKeyframe = (nalVVC->header.nal_unit_type == NalType::IDR_W_RADL ||
                                             nalVVC->header.nal_unit_type == NalType::IDR_N_LP ||
                                             nalVVC->header.nal_unit_type == NalType::CRA_NUT);
  if (updatedParsingState.lastFrameIsKeyframe)
  {
    nalVVC->rawData = data;
    this->nalUnitsForSeeking.push_back(nalVVC);
  }

  if (this->auDelimiterDetector.isStartOfNewAU(nalVVC,
                                               updatedParsingState.currentPictureHeaderStructure))
  {
    if (!this->handleNewAU(updatedParsingState, parseResult, bitrateEntry, nalStartEndPosFile))
    {
      specificDescription +=
          " ERROR Adding POC " + std::to_string(this->parsingState.lastFramePOC) + " to frame list";
      parseResult.success = false;
    }
  }
  else if (this->parsingState.curFrameFileStartEndPos && nalStartEndPosFile)
    updatedParsingState.curFrameFileStartEndPos->second = nalStartEndPosFile->second;

  this->parsingState = updatedParsingState;
  this->parsingState.sizeCurrentAU += data.size();

  if (nalRoot)
  {
    auto name = "NAL " + std::to_string(nalVVC->nalIdx) + ": " +
                std::to_string(nalVVC->header.nalUnitTypeID) + specificDescription;
    nalRoot->setProperties(name);
  }

  return parseResult;
}

// 7.4.2.4.3
bool AnnexBVVC::auDelimiterDetector_t::isStartOfNewAU(
    std::shared_ptr<vvc::NalUnitVVC> nal, std::shared_ptr<vvc::picture_header_structure> ph)
{
  if (!nal || !ph)
    return false;

  auto nalType = nal->header.nal_unit_type;

  if (this->lastNalWasVcl &&
      (nalType == NalType::AUD_NUT || nalType == NalType::OPI_NUT || nalType == NalType::DCI_NUT ||
       nalType == NalType::VPS_NUT || nalType == NalType::SPS_NUT || nalType == NalType::PPS_NUT ||
       nalType == NalType::PREFIX_APS_NUT || nalType == NalType::PH_NUT ||
       nalType == NalType::PREFIX_SEI_NUT || nalType == NalType::RSV_NVCL_26 ||
       nalType == NalType::UNSPEC_28 || nalType == NalType::UNSPEC_29))
  {
    this->lastNalWasVcl = false;
    return true;
  }

  auto isSlice = (nalType == NalType::TRAIL_NUT || nalType == NalType::STSA_NUT ||
                  nalType == NalType::RADL_NUT || nalType == NalType::RASL_NUT ||
                  nalType == NalType::IDR_W_RADL || nalType == NalType::IDR_N_LP ||
                  nalType == NalType::CRA_NUT || nalType == NalType::GDR_NUT);

  auto isVcl = (isSlice || nalType == NalType::RSV_VCL_4 || nalType == NalType::RSV_VCL_5 ||
                nalType == NalType::RSV_VCL_6 || nalType == NalType::RSV_IRAP_11);

  if (isVcl && this->lastNalWasVcl)
  {
    if (nal->header.nuh_layer_id != this->lastVcl_nuh_layer_id ||
        ph->ph_pic_order_cnt_lsb != this->lastVcl_ph_pic_order_cnt_lsb ||
        ph->PicOrderCntVal != this->lastVcl_PicOrderCntVal)
    {
      this->lastVcl_nuh_layer_id         = nal->header.nuh_layer_id;
      this->lastVcl_ph_pic_order_cnt_lsb = ph->ph_pic_order_cnt_lsb;
      this->lastVcl_PicOrderCntVal       = ph->PicOrderCntVal;
      return true;
    }
  }

  this->lastNalWasVcl = isVcl;
  return false;
}

bool AnnexBVVC::handleNewAU(ParsingState &              updatedParsingState,
                            AnnexB::ParseResult &       parseResult,
                            std::optional<BitrateEntry> bitrateEntry,
                            std::optional<pairUint64>   nalStartEndPosFile)
{
  DEBUG_VVC("Start of new AU. Adding bitrate " << this->parsingState.sizeCurrentAU << " POC "
                                               << this->parsingState.lastFramePOC << " AU "
                                               << this->parsingState.counterAU);

  BitrateEntry entry;
  if (bitrateEntry)
  {
    entry.pts      = bitrateEntry->pts;
    entry.dts      = bitrateEntry->dts;
    entry.duration = bitrateEntry->duration;
  }
  else
  {
    entry.pts      = this->parsingState.lastFramePOC;
    entry.dts      = int(this->parsingState.counterAU);
    entry.duration = 1;
  }
  entry.bitrate            = unsigned(this->parsingState.sizeCurrentAU);
  entry.keyframe           = this->parsingState.lastFrameIsKeyframe;
  parseResult.bitrateEntry = entry;

  if (!addFrameToList(this->parsingState.lastFramePOC,
                      this->parsingState.curFrameFileStartEndPos,
                      this->parsingState.lastFrameIsKeyframe))
  {
    return false;
  }
  if (this->parsingState.curFrameFileStartEndPos)
    DEBUG_VVC("Adding start/end " << this->parsingState.curFrameFileStartEndPos->first << "/"
                                  << this->parsingState.curFrameFileStartEndPos->second << " - AU "
                                  << this->parsingState.counterAU
                                  << (this->parsingState.lastFrameIsKeyframe ? " - ra" : ""));
  else
    DEBUG_VVC("Adding start/end %d/%d - POC NA/NA"
              << (this->parsingState.lastFrameIsKeyframe ? " - ra" : ""));

  updatedParsingState.curFrameFileStartEndPos = nalStartEndPosFile;
  updatedParsingState.sizeCurrentAU           = 0;
  updatedParsingState.counterAU++;

  return true;
}

} // namespace parser
