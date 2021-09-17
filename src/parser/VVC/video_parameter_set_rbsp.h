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

#include "NalUnitVVC.h"
#include "dpb_parameters.h"
#include "general_timing_hrd_parameters.h"
#include "ols_timing_hrd_parameters.h"
#include "parser/common/SubByteReaderLogging.h"
#include "profile_tier_level.h"
#include "rbsp_trailing_bits.h"

namespace parser::vvc
{

class video_parameter_set_rbsp : public NalRBSP
{
public:
  video_parameter_set_rbsp()  = default;
  ~video_parameter_set_rbsp() = default;
  void parse(reader::SubByteReaderLogging &reader);

  unsigned                      vps_video_parameter_set_id{};
  unsigned                      vps_max_layers_minus1{};
  unsigned                      vps_max_sublayers_minus1{};
  bool                          vps_default_ptl_dpb_hrd_max_tid_flag{true};
  bool                          vps_all_independent_layers_flag{true};
  vector<unsigned>              vps_layer_id{};
  vector<bool>                  vps_independent_layer_flag{};
  vector<bool>                  vps_max_tid_ref_present_flag{};
  vector2d<bool>                vps_direct_ref_layer_flag{};
  vector2d<unsigned>            vps_max_tid_il_ref_pics_plus1{};
  bool                          vps_each_layer_is_an_ols_flag{};
  unsigned                      vps_ols_mode_idc{};
  unsigned                      vps_num_output_layer_sets_minus2{};
  vector2d<bool>                vps_ols_output_layer_flag{};
  unsigned                      vps_num_ptls_minus1{};
  vector<bool>                  vps_pt_present_flag{};
  vector<unsigned>              vps_ptl_max_tid{};
  bool                          vps_ptl_alignment_zero_bit{};
  profile_tier_level            profile_tier_level_instance;
  vector<unsigned>              vps_ols_ptl_idx{};
  unsigned                      vps_num_dpb_params_minus1{};
  bool                          vps_sublayer_dpb_params_present_flag{};
  vector<unsigned>              vps_dpb_max_tid{};
  dpb_parameters                dpb_parameters_instance;
  vector<unsigned>              vps_ols_dpb_pic_width{};
  vector<unsigned>              vps_ols_dpb_pic_height{};
  vector<unsigned>              vps_ols_dpb_chroma_format{};
  vector<unsigned>              vps_ols_dpb_bitdepth_minus8{};
  vector<unsigned>              vps_ols_dpb_params_idx{};
  bool                          vps_timing_hrd_params_present_flag{};
  general_timing_hrd_parameters general_timing_hrd_parameters_instance;
  bool                          vps_sublayer_cpb_params_present_flag{};
  unsigned                      vps_num_ols_timing_hrd_params_minus1{};
  vector<unsigned>              vps_hrd_max_tid{};
  ols_timing_hrd_parameters     ols_timing_hrd_parameters_instance;
  vector<unsigned>              vps_ols_timing_hrd_idx{};
  bool                          vps_extension_flag{};
  bool                          vps_extension_data_flag{};
  rbsp_trailing_bits            rbsp_trailing_bits_instance;

  umap_1d<bool>     LayerUsedAsRefLayerFlag;
  umap_2d<unsigned> DirectRefLayerIdx;
  umap_2d<unsigned> ReferenceLayerIdx;
  umap_1d<unsigned> NumDirectRefLayers;
  umap_1d<unsigned> NumRefLayers;
  unsigned          olsModeIdc{};
  unsigned          TotalNumOlss{};
  umap_1d<unsigned> NumOutputLayersInOls;
  umap_2d<unsigned> OutputLayerIdInOls;
  umap_2d<unsigned> NumSubLayersInLayerInOLS;
  umap_1d<bool>     LayerUsedAsOutputLayerFlag;
  umap_2d<bool>     layerIncludedInOlsFlag;
  umap_1d<unsigned> NumLayersInOls;
  umap_2d<unsigned> LayerIdInOls;
  umap_1d<unsigned> MultiLayerOlsIdx;
  unsigned          NumMultiLayerOlss;
  unsigned          VpsNumDpbParams{};
};

} // namespace parser::vvc
