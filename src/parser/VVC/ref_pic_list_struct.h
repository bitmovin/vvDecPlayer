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

class seq_parameter_set_rbsp;

class ref_pic_list_struct
{
public:
  ref_pic_list_struct()  = default;
  ~ref_pic_list_struct() = default;
  void parse(reader::SubByteReaderLogging &          reader,
             unsigned                                listIdx,
             unsigned                                rplsIdx,
             std::shared_ptr<seq_parameter_set_rbsp> sps);

  unsigned          num_ref_entries{};
  bool              ltrp_in_header_flag{};
  umap_1d<bool>     inter_layer_ref_pic_flag{};
  umap_1d<bool>     st_ref_pic_flag{};
  umap_1d<unsigned> abs_delta_poc_st{};
  umap_1d<bool>     strp_entry_sign_flag{};
  umap_1d<unsigned> rpls_poc_lsb_lt{};
  umap_1d<unsigned> ilrp_idx{};

  unsigned NumLtrpEntries;

  umap_1d<unsigned> AbsDeltaPocSt;
  umap_1d<int>      DeltaPocValSt;

private:
  bool getStRefPicFlag(unsigned i);
};

} // namespace parser::vvc
