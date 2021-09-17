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

#include "sublayer_hrd_parameters.h"

#include "general_timing_hrd_parameters.h"

namespace parser::vvc
{

using namespace parser::reader;

void sublayer_hrd_parameters::parse(SubByteReaderLogging &              reader,
                                    unsigned                       subLayerId,
                                    general_timing_hrd_parameters *general_hrd)
{
  assert(general_hrd != nullptr);
  SubByteReaderLoggingSubLevel subLevel(reader, "sublayer_hrd_parameters");

  for (unsigned j = 0; j <= general_hrd->hrd_cpb_cnt_minus1; j++)
  {
    this->bit_rate_value_minus1[subLayerId][j] = reader.readUEV("bit_rate_value_minus1");
    this->cpb_size_value_minus1[subLayerId][j] = reader.readUEV("cpb_size_value_minus1");
    if (general_hrd->general_du_hrd_params_present_flag)
    {
      this->cpb_size_du_value_minus1[subLayerId][j] = reader.readUEV("cpb_size_du_value_minus1");
      this->bit_rate_du_value_minus1[subLayerId][j] = reader.readUEV("bit_rate_du_value_minus1");
    }
    this->cbr_flag[subLayerId][j] = reader.readFlag("cbr_flag");
  }
}

} // namespace parser::vvc
