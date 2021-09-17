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

#include "lmcs_data.h"

#include "adaptation_parameter_set_rbsp.h"
#include "parser/common/functions.h"

namespace parser::vvc
{

using namespace parser::reader;

void lmcs_data::parse(SubByteReaderLogging &reader, adaptation_parameter_set_rbsp *aps)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "lmcs_data");

  this->lmcs_min_bin_idx = reader.readUEV("lmcs_min_bin_idx", Options().withCheckRange({0, 15}));
  this->lmcs_delta_max_bin_idx =
      reader.readUEV("lmcs_delta_max_bin_idx", Options().withCheckRange({0, 15}));
  this->LmcsMaxBinIdx = 15 - lmcs_delta_max_bin_idx;
  this->lmcs_delta_cw_prec_minus1 =
      reader.readUEV("lmcs_delta_cw_prec_minus1", Options().withCheckRange({0, 14}));
  for (unsigned i = lmcs_min_bin_idx; i <= this->LmcsMaxBinIdx; i++)
  {
    auto nrBits                = this->lmcs_delta_cw_prec_minus1 + 1;
    this->lmcs_delta_abs_cw[i] = reader.readBits(formatArray("lmcs_delta_abs_cw", i), nrBits);
    if (this->lmcs_delta_abs_cw[i] > 0)
    {
      this->lmcs_delta_sign_cw_flag[i] = reader.readFlag(formatArray("lmcs_delta_sign_cw_flag", i));
    }
  }
  if (aps->aps_chroma_present_flag)
  {
    this->lmcs_delta_abs_crs = reader.readBits("lmcs_delta_abs_crs", 3);
    if (this->lmcs_delta_abs_crs > 0)
    {
      this->lmcs_delta_sign_crs_flag = reader.readFlag("lmcs_delta_sign_crs_flag");
    }
  }
}

} // namespace parser::vvc
