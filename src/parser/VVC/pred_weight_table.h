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
#include "parser/common/SubByteReaderLogging.h"

namespace parser::vvc
{

class seq_parameter_set_rbsp;
class pic_parameter_set_rbsp;
class slice_layer_rbsp;
class ref_pic_lists;

class pred_weight_table : public NalRBSP
{
public:
  pred_weight_table()  = default;
  ~pred_weight_table() = default;
  void parse(reader::SubByteReaderLogging &               reader,
             std::shared_ptr<seq_parameter_set_rbsp> sps,
             std::shared_ptr<pic_parameter_set_rbsp> pps,
             std::shared_ptr<slice_layer_rbsp>       sl,
             std::shared_ptr<ref_pic_lists>          rpl);

  unsigned      luma_log2_weight_denom{};
  int           delta_chroma_log2_weight_denom{};
  unsigned      num_l0_weights{};
  vector<bool>  luma_weight_l0_flag{};
  vector<bool>  chroma_weight_l0_flag{};
  vector<int>   delta_luma_weight_l0{};
  vector<int>   luma_offset_l0{};
  umap_2d<int>  delta_chroma_weight_l0{};
  vector2d<int> delta_chroma_offset_l0{};
  unsigned      num_l1_weights{};
  vector<bool>  luma_weight_l1_flag{};
  vector<bool>  chroma_weight_l1_flag{};
  vector<int>   delta_luma_weight_l1{};
  vector<int>   luma_offset_l1{};
  vector2d<int> delta_chroma_weight_l1{};
  vector2d<int> delta_chroma_offset_l1{};
};

} // namespace parser::vvc
