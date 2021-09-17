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

class alf_data : public NalRBSP
{
public:
  alf_data()  = default;
  ~alf_data() = default;
  void parse(reader::SubByteReaderLogging &reader, adaptation_parameter_set_rbsp *aps);

  bool               alf_luma_filter_signal_flag{};
  bool               alf_chroma_filter_signal_flag{};
  bool               alf_cc_cb_filter_signal_flag{};
  bool               alf_cc_cr_filter_signal_flag{};
  bool               alf_luma_clip_flag{};
  unsigned           alf_luma_num_filters_signalled_minus1{};
  int                alf_luma_coeff_delta_idx{};
  vector2d<unsigned> alf_luma_coeff_abs{};
  vector2d<bool>     alf_luma_coeff_sign{};
  vector2d<unsigned> alf_luma_clip_idx{};
  bool               alf_chroma_clip_flag{};
  unsigned           alf_chroma_num_alt_filters_minus1{};
  vector2d<unsigned> alf_chroma_coeff_abs{};
  vector2d<bool>     alf_chroma_coeff_sign{};
  vector2d<unsigned> alf_chroma_clip_idx{};
  unsigned           alf_cc_cb_filters_signalled_minus1{};
  vector2d<unsigned> alf_cc_cb_mapped_coeff_abs{};
  vector2d<bool>     alf_cc_cb_coeff_sign{};
  unsigned           alf_cc_cr_filters_signalled_minus1{};
  vector2d<unsigned> alf_cc_cr_mapped_coeff_abs{};
  vector2d<bool>     alf_cc_cr_coeff_sign{};
};

} // namespace parser::vvc
