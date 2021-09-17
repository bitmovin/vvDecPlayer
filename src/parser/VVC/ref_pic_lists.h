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
#include "ref_pic_list_struct.h"

namespace parser::vvc
{

class seq_parameter_set_rbsp;
class pic_parameter_set_rbsp;

class ref_pic_lists : public NalRBSP
{
public:
  ref_pic_lists()  = default;
  ~ref_pic_lists() = default;
  void parse(reader::SubByteReaderLogging &          reader,
             std::shared_ptr<seq_parameter_set_rbsp> sps,
             std::shared_ptr<pic_parameter_set_rbsp> pps);

  ref_pic_list_struct getActiveRefPixList(std::shared_ptr<seq_parameter_set_rbsp> sps, unsigned listIndex) const;

  umap_1d<bool>       rpl_sps_flag{};
  umap_1d<int>        rpl_idx{};
  ref_pic_list_struct ref_pic_list_structs[2];
  int                 poc_lsb_lt{};
  vector2d<bool>      delta_poc_msb_cycle_present_flag{};
  umap_2d<unsigned>   delta_poc_msb_cycle_lt{};

  umap_1d<int> RplsIdx;
};

} // namespace parser::vvc
