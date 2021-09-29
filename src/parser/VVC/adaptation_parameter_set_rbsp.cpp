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

#include "adaptation_parameter_set_rbsp.h"

namespace parser::vvc
{

using namespace parser::reader;

void adaptation_parameter_set_rbsp::parse(SubByteReaderLogging &reader)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "adaptation_parameter_set_rbsp");

  auto aps_params_type_ID =
      reader.readBits("aps_params_type",
                      3,
                      Options().withCheckRange({0, 2}).withMeaningVector(
                          {"ALF parameters", "LMCS parameters", "Scaling list parameters"}));
  this->aps_params_type = *apsParamTypeMapper.at(aps_params_type_ID);

  this->aps_adaptation_parameter_set_id =
      reader.readBits("aps_adaptation_parameter_set_id", 5, Options().withCheckRange({0, 7}));
  this->aps_chroma_present_flag = reader.readFlag("aps_chroma_present_flag");
  if (this->aps_params_type == APSParamType::ALF_APS)
  {
    this->alf_data_instance.parse(reader, this);
  }
  else if (this->aps_params_type == APSParamType::LMCS_APS)
  {
    this->lmcs_data_instance.parse(reader, this);
  }
  else if (this->aps_params_type == APSParamType::SCALING_APS)
  {
    this->scaling_list_data_instance.parse(reader, this);
  }
  this->aps_extension_flag = reader.readFlag("aps_extension_flag");
  if (this->aps_extension_flag)
  {
    while (reader.more_rbsp_data())
    {
      this->aps_extension_data_flag = reader.readFlag("aps_extension_data_flag");
    }
  }
  this->rbsp_trailing_bits_instance.parse(reader);
}

} // namespace parser::vvc
