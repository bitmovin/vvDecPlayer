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
#include "byte_alignment.h"
#include "parser/common/SubByteReaderLogging.h"
#include "picture_header_structure.h"
#include "pred_weight_table.h"
#include "ref_pic_lists.h"

#include <memory>

namespace parser::vvc
{

enum class SliceType
{
  B,
  P,
  I
};

std::string to_string(SliceType sliceType);

class slice_header : public NalRBSP
{
public:
  slice_header()  = default;
  ~slice_header() = default;
  void parse(reader::SubByteReaderLogging &                 reader,
             NalType                                   nal_unit_type,
             VPSMap &                                  vpsMap,
             SPSMap &                                  spsMap,
             PPSMap &                                  ppsMap,
             std::shared_ptr<slice_layer_rbsp>         sliceLayer,
             std::shared_ptr<picture_header_structure> picHeader);

  bool                                      sh_picture_header_in_slice_header_flag{};
  std::shared_ptr<picture_header_structure> picture_header_structure_instance;

  unsigned                       sh_subpic_id{};
  unsigned                       sh_slice_address{};
  vector<bool>                   sh_extra_bit{};
  unsigned                       sh_num_tiles_in_slice_minus1{};
  SliceType                      sh_slice_type{SliceType::I};
  bool                           sh_no_output_of_prior_pics_flag{};
  bool                           sh_alf_enabled_flag{};
  unsigned                       sh_num_alf_aps_ids_luma{};
  vector<unsigned>               sh_alf_aps_id_luma{};
  bool                           sh_alf_cb_enabled_flag{};
  bool                           sh_alf_cr_enabled_flag{};
  unsigned                       sh_alf_aps_id_chroma{};
  bool                           sh_alf_cc_cb_enabled_flag{};
  unsigned                       sh_alf_cc_cb_aps_id{};
  bool                           sh_alf_cc_cr_enabled_flag{};
  unsigned                       sh_alf_cc_cr_aps_id{};
  bool                           sh_lmcs_used_flag{};
  bool                           sh_explicit_scaling_list_used_flag{};
  std::shared_ptr<ref_pic_lists> ref_pic_lists_instance;
  bool                           sh_num_ref_idx_active_override_flag{};
  umap_1d<unsigned>              sh_num_ref_idx_active_minus1{};
  bool                           sh_cabac_init_flag{};
  bool                           sh_collocated_from_l0_flag{};
  unsigned                       sh_collocated_ref_idx{};
  pred_weight_table              pred_weight_table_instance;
  int                            sh_qp_delta{};
  int                            sh_cb_qp_offset{};
  int                            sh_cr_qp_offset{};
  int                            sh_joint_cbcr_qp_offset{};
  bool                           sh_cu_chroma_qp_offset_enabled_flag{};
  bool                           sh_sao_luma_used_flag{};
  bool                           sh_sao_chroma_used_flag{};
  bool                           sh_deblocking_params_present_flag{};
  bool                           sh_deblocking_filter_disabled_flag{};
  int                            sh_luma_beta_offset_div2{};
  int                            sh_luma_tc_offset_div2{};
  int                            sh_cb_beta_offset_div2{};
  int                            sh_cb_tc_offset_div2{};
  int                            sh_cr_beta_offset_div2{};
  int                            sh_cr_tc_offset_div2{};
  bool                           sh_dep_quant_used_flag{};
  bool                           sh_sign_data_hiding_used_flag{};
  bool                           sh_ts_residual_coding_disabled_flag{};
  unsigned                       sh_slice_header_extension_length{};
  vector<unsigned>               sh_slice_header_extension_data_byte{};
  unsigned                       sh_entry_offset_len_minus1{};
  int                            sh_entry_point_offset_minus1{};
  byte_alignment                 byte_alignment_instance;

  vector<unsigned> NumRefIdxActive;
};

} // namespace parser::vvc
