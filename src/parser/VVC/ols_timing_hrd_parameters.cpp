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

#include "ols_timing_hrd_parameters.h"

#include "general_timing_hrd_parameters.h"

#include <assert.h>

namespace parser::vvc
{

using namespace parser::reader;

void ols_timing_hrd_parameters::parse(SubByteReaderLogging &              reader,
                                      unsigned                       firstSubLayer,
                                      unsigned                       MaxSubLayersVal,
                                      general_timing_hrd_parameters *general_hrd)
{
  assert(general_hrd != nullptr);
  SubByteReaderLoggingSubLevel subLevel(reader, "ols_timing_hrd_parameters");

  for (unsigned i = firstSubLayer; i <= MaxSubLayersVal; i++)
  {
    this->fixed_pic_rate_general_flag.push_back(reader.readFlag("fixed_pic_rate_general_flag"));
    if (!this->fixed_pic_rate_general_flag[i])
    {
      this->fixed_pic_rate_within_cvs_flag.push_back(
          reader.readFlag("fixed_pic_rate_within_cvs_flag"));
    }
    if (this->fixed_pic_rate_within_cvs_flag[i])
    {
      this->elemental_duration_in_tc_minus1.push_back(
          reader.readUEV("elemental_duration_in_tc_minus1"));
    }
    else if ((general_hrd->general_nal_hrd_params_present_flag || general_hrd->general_vcl_hrd_params_present_flag) &&
             general_hrd->hrd_cpb_cnt_minus1 == 0)
    {
      this->low_delay_hrd_flag.push_back(reader.readFlag("low_delay_hrd_flag"));
    }
    if (general_hrd->general_nal_hrd_params_present_flag)
    {
      this->sublayer_hrd_parameters_nal.parse(reader, i, general_hrd);
    }
    if (general_hrd->general_vcl_hrd_params_present_flag)
    {
      this->sublayer_hrd_parameters_vcl.parse(reader, i, general_hrd);
    }
  }
}

} // namespace parser::vvc
