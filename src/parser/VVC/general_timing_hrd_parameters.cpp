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

#include "general_timing_hrd_parameters.h"

namespace parser::vvc
{

using namespace parser::reader;

void general_timing_hrd_parameters::parse(SubByteReaderLogging &reader)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "general_timing_hrd_parameters");

  this->num_units_in_tick = reader.readBits("num_units_in_tick", 32, Options().withCheckGreater(0));
  this->time_scale        = reader.readBits("time_scale", 32, Options().withCheckGreater(0));
  this->general_nal_hrd_params_present_flag =
      reader.readFlag("general_nal_hrd_params_present_flag");
  this->general_vcl_hrd_params_present_flag =
      reader.readFlag("general_vcl_hrd_params_present_flag");
  if (this->general_nal_hrd_params_present_flag || this->general_vcl_hrd_params_present_flag)
  {
    this->general_same_pic_timing_in_all_ols_flag =
        reader.readFlag("general_same_pic_timing_in_all_ols_flag");
    this->general_du_hrd_params_present_flag =
        reader.readFlag("general_du_hrd_params_present_flag");
    if (this->general_du_hrd_params_present_flag)
    {
      this->tick_divisor_minus2 = reader.readBits("tick_divisor_minus2", 8);
    }
    this->bit_rate_scale = reader.readBits("bit_rate_scale", 4);
    this->cpb_size_scale = reader.readBits("cpb_size_scale", 4);
    if (this->general_du_hrd_params_present_flag)
    {
      this->cpb_size_du_scale = reader.readBits("cpb_size_du_scale", 4);
    }
    this->hrd_cpb_cnt_minus1 =
        reader.readUEV("hrd_cpb_cnt_minus1", Options().withCheckRange({0, 31}));
  }
}

} // namespace parser::vvc
