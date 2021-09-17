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
#include "alf_data.h"
#include "lmcs_data.h"
#include "parser/common/SubByteReaderLogging.h"
#include "rbsp_trailing_bits.h"
#include "scaling_list_data.h"


namespace parser::vvc
{

class adaptation_parameter_set_rbsp : public NalRBSP
{
public:
  adaptation_parameter_set_rbsp()  = default;
  ~adaptation_parameter_set_rbsp() = default;
  void parse(reader::SubByteReaderLogging &reader);

  enum class APSParamType
  {
    ALF_APS,
    LMCS_APS,
    SCALING_APS
  };

  APSParamType       aps_params_type{};
  unsigned           aps_adaptation_parameter_set_id{};
  bool               aps_chroma_present_flag{};
  alf_data           alf_data_instance;
  lmcs_data          lmcs_data_instance;
  scaling_list_data  scaling_list_data_instance;
  bool               aps_extension_flag{};
  bool               aps_extension_data_flag{};
  rbsp_trailing_bits rbsp_trailing_bits_instance;
};

} // namespace parser::vvc
