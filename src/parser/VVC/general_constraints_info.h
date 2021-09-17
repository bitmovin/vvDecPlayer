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

#include "parser/common/SubByteReaderLogging.h"

namespace parser::vvc
{

class general_constraints_info
{
public:
  general_constraints_info()  = default;
  ~general_constraints_info() = default;
  void parse(reader::SubByteReaderLogging &reader);

  bool gci_present_flag{};
  bool gci_intra_only_constraint_flag{};
  bool gci_all_layers_independent_constraint_flag{};
  bool gci_one_au_only_constraint_flag{};
  ///* picture format */
  unsigned gci_sixteen_minus_max_bitdepth_constraint_idc{};
  unsigned gci_three_minus_max_chroma_format_constraint_idc{};
  ///* NAL unit type related */
  bool gci_no_mixed_nalu_types_in_pic_constraint_flag{};
  bool gci_no_trail_constraint_flag{};
  bool gci_no_stsa_constraint_flag{};
  bool gci_no_rasl_constraint_flag{};
  bool gci_no_radl_constraint_flag{};
  bool gci_no_idr_constraint_flag{};
  bool gci_no_cra_constraint_flag{};
  bool gci_no_gdr_constraint_flag{};
  bool gci_no_aps_constraint_flag{};
  bool gci_no_idr_rpl_constraint_flag{};
  ///* tile, slice, subpicture partitioning */
  bool gci_one_tile_per_pic_constraint_flag{};
  bool gci_pic_header_in_slice_header_constraint_flag{};
  bool gci_one_slice_per_pic_constraint_flag{};
  bool gci_no_rectangular_slice_constraint_flag{};
  bool gci_one_slice_per_subpic_constraint_flag{};
  bool gci_no_subpic_info_constraint_flag{};
  ///* CTU and block partitioning */
  unsigned gci_three_minus_max_log2_ctu_size_constraint_idc{};
  bool     gci_no_partition_constraints_override_constraint_flag{};
  bool     gci_no_mtt_constraint_flag{};
  bool     gci_no_qtbtt_dual_tree_intra_constraint_flag{};
  ///* intra */
  bool gci_no_palette_constraint_flag{};
  bool gci_no_ibc_constraint_flag{};
  bool gci_no_isp_constraint_flag{};
  bool gci_no_mrl_constraint_flag{};
  bool gci_no_mip_constraint_flag{};
  bool gci_no_cclm_constraint_flag{};
  ///* inter */
  bool gci_no_ref_pic_resampling_constraint_flag{};
  bool gci_no_res_change_in_clvs_constraint_flag{};
  bool gci_no_weighted_prediction_constraint_flag{};
  bool gci_no_ref_wraparound_constraint_flag{};
  bool gci_no_temporal_mvp_constraint_flag{};
  bool gci_no_sbtmvp_constraint_flag{};
  bool gci_no_amvr_constraint_flag{};
  bool gci_no_bdof_constraint_flag{};
  bool gci_no_smvd_constraint_flag{};
  bool gci_no_dmvr_constraint_flag{};
  bool gci_no_mmvd_constraint_flag{};
  bool gci_no_affine_motion_constraint_flag{};
  bool gci_no_prof_constraint_flag{};
  bool gci_no_bcw_constraint_flag{};
  bool gci_no_ciip_constraint_flag{};
  bool gci_no_gpm_constraint_flag{};
  ///* transform, quantization, residual */
  bool gci_no_luma_transform_size_64_constraint_flag{};
  bool gci_no_transform_skip_constraint_flag{};
  bool gci_no_bdpcm_constraint_flag{};
  bool gci_no_mts_constraint_flag{};
  bool gci_no_lfnst_constraint_flag{};
  bool gci_no_joint_cbcr_constraint_flag{};
  bool gci_no_sbt_constraint_flag{};
  bool gci_no_act_constraint_flag{};
  bool gci_no_explicit_scaling_list_constraint_flag{};
  bool gci_no_dep_quant_constraint_flag{};
  bool gci_no_sign_data_hiding_constraint_flag{};
  bool gci_no_cu_qp_delta_constraint_flag{};
  bool gci_no_chroma_qp_offset_constraint_flag{};
  ///* loop filter */
  bool              gci_no_sao_constraint_flag{};
  bool              gci_no_alf_constraint_flag{};
  bool              gci_no_ccalf_constraint_flag{};
  bool              gci_no_lmcs_constraint_flag{};
  bool              gci_no_ladf_constraint_flag{};
  bool              gci_no_virtual_boundaries_constraint_flag{};
  unsigned          gci_num_reserved_bits{};
  std::vector<bool> gci_reserved_zero_bit{};
  bool              gci_alignment_zero_bit{};
};

} // namespace parser::vvc
