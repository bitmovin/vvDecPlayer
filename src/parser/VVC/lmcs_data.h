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

class adaptation_parameter_set_rbsp;

class lmcs_data : public NalRBSP
{
public:
  lmcs_data()  = default;
  ~lmcs_data() = default;
  void parse(reader::SubByteReaderLogging &reader, adaptation_parameter_set_rbsp *aps);

  unsigned      lmcs_min_bin_idx{};
  unsigned      lmcs_delta_max_bin_idx{};
  unsigned      lmcs_delta_cw_prec_minus1{};
  umap_1d<int>  lmcs_delta_abs_cw;
  umap_1d<bool> lmcs_delta_sign_cw_flag;
  unsigned      lmcs_delta_abs_crs{};
  bool          lmcs_delta_sign_crs_flag{};

  unsigned LmcsMaxBinIdx{};
};

} // namespace parser::vvc
