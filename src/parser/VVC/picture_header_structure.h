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
#include "commonMaps.h"
#include "parser/common/SubByteReaderLogging.h"
#include "pred_weight_table.h"
#include "ref_pic_lists.h"

#include <memory>

namespace parser::vvc
{

class seq_parameter_set_rbsp;
class pic_parameter_set_rbsp;
class slice_layer_rbsp;

class picture_header_structure : public NalRBSP
{
public:
  picture_header_structure()  = default;
  ~picture_header_structure() = default;
  void parse(reader::SubByteReaderLogging &    reader,
             VPSMap &                          vpsMap,
             SPSMap &                          spsMap,
             PPSMap &                          ppsMap,
             std::shared_ptr<slice_layer_rbsp> sl);

  void calculatePictureOrderCount(reader::SubByteReaderLogging &            reader,
                                  NalType                                   nalType,
                                  SPSMap &                                  spsMap,
                                  PPSMap &                                  ppsMap,
                                  std::shared_ptr<picture_header_structure> previousPicture);

  bool                           ph_gdr_or_irap_pic_flag{};
  bool                           ph_non_ref_pic_flag{};
  bool                           ph_gdr_pic_flag{};
  bool                           ph_inter_slice_allowed_flag{};
  bool                           ph_intra_slice_allowed_flag{true};
  unsigned                       ph_pic_parameter_set_id{};
  unsigned                       ph_pic_order_cnt_lsb{};
  unsigned                       ph_recovery_poc_cnt{};
  vector<bool>                   ph_extra_bit{};
  bool                           ph_poc_msb_cycle_present_flag{};
  unsigned                       ph_poc_msb_cycle_val{};
  bool                           ph_alf_enabled_flag{};
  unsigned                       ph_num_alf_aps_ids_luma{};
  vector<unsigned>               ph_alf_aps_id_luma{};
  bool                           ph_alf_cb_enabled_flag{};
  bool                           ph_alf_cr_enabled_flag{};
  unsigned                       ph_alf_aps_id_chroma{};
  bool                           ph_alf_cc_cb_enabled_flag{};
  unsigned                       ph_alf_cc_cb_aps_id{};
  bool                           ph_alf_cc_cr_enabled_flag{};
  unsigned                       ph_alf_cc_cr_aps_id{};
  bool                           ph_lmcs_enabled_flag{};
  unsigned                       ph_lmcs_aps_id{};
  bool                           ph_chroma_residual_scale_flag{};
  bool                           ph_explicit_scaling_list_enabled_flag{};
  unsigned                       ph_scaling_list_aps_id{};
  bool                           ph_virtual_boundaries_present_flag{};
  unsigned                       ph_num_ver_virtual_boundaries{};
  vector<unsigned>               ph_virtual_boundary_pos_x_minus1{};
  unsigned                       ph_num_hor_virtual_boundaries{};
  vector<unsigned>               ph_virtual_boundary_pos_y_minus1{};
  bool                           ph_pic_output_flag{true};
  std::shared_ptr<ref_pic_lists> ref_pic_lists_instance;
  bool                           ph_partition_constraints_override_flag{};
  unsigned                       ph_log2_diff_min_qt_min_cb_intra_slice_luma{};
  unsigned                       ph_max_mtt_hierarchy_depth_intra_slice_luma{};
  unsigned                       ph_log2_diff_max_bt_min_qt_intra_slice_luma{};
  unsigned                       ph_log2_diff_max_tt_min_qt_intra_slice_luma{};
  unsigned                       ph_log2_diff_min_qt_min_cb_intra_slice_chroma{};
  unsigned                       ph_max_mtt_hierarchy_depth_intra_slice_chroma{};
  unsigned                       ph_log2_diff_max_bt_min_qt_intra_slice_chroma{};
  unsigned                       ph_log2_diff_max_tt_min_qt_intra_slice_chroma{};
  unsigned                       ph_cu_qp_delta_subdiv_intra_slice{};
  unsigned                       ph_cu_chroma_qp_offset_subdiv_intra_slice{};
  unsigned                       ph_log2_diff_min_qt_min_cb_inter_slice{};
  unsigned                       ph_max_mtt_hierarchy_depth_inter_slice{};
  unsigned                       ph_log2_diff_max_bt_min_qt_inter_slice{};
  unsigned                       ph_log2_diff_max_tt_min_qt_inter_slice{};
  unsigned                       ph_cu_qp_delta_subdiv_inter_slice{};
  unsigned                       ph_cu_chroma_qp_offset_subdiv_inter_slice{};
  bool                           ph_temporal_mvp_enabled_flag{};
  bool                           ph_collocated_from_l0_flag{};
  unsigned                       ph_collocated_ref_idx{};
  bool                           ph_mmvd_fullpel_only_flag{};
  bool                           ph_mvd_l1_zero_flag{};
  bool                           ph_bdof_disabled_flag{};
  bool                           ph_dmvr_disabled_flag{};
  bool                           ph_prof_disabled_flag{};
  pred_weight_table              pred_weight_table_instance;
  int                            ph_qp_delta{};
  bool                           ph_joint_cbcr_sign_flag{};
  bool                           ph_sao_luma_enabled_flag{};
  bool                           ph_sao_chroma_enabled_flag{};
  bool                           ph_deblocking_params_present_flag{};
  bool                           ph_deblocking_filter_disabled_flag{};
  int                            ph_luma_beta_offset_div2{};
  int                            ph_luma_tc_offset_div2{};
  int                            ph_cb_beta_offset_div2{};
  int                            ph_cb_tc_offset_div2{};
  int                            ph_cr_beta_offset_div2{};
  int                            ph_cr_tc_offset_div2{};
  unsigned                       ph_extension_length{};
  vector<unsigned>               ph_extension_data_byte{};

  unsigned PicOrderCntMsb{};
  unsigned PicOrderCntVal{};
};

} // namespace parser::vvc
