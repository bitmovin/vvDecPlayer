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

#include "operating_point_information_rbsp.h"

namespace parser::vvc
{

using namespace parser::reader;

void operating_point_information_rbsp::parse(SubByteReaderLogging &reader)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "operating_point_information_rbsp");

  this->opi_ols_info_present_flag  = reader.readFlag("opi_ols_info_present_flag");
  this->opi_htid_info_present_flag = reader.readFlag("opi_htid_info_present_flag");
  if (this->opi_ols_info_present_flag)
  {
    this->opi_ols_idx = reader.readUEV("opi_ols_idx");
  }
  if (this->opi_htid_info_present_flag)
  {
    this->opi_htid_plus1 = reader.readBits("opi_htid_plus1", 3);
  }
  this->opi_extension_flag = reader.readFlag("opi_extension_flag", Options().withCheckEqualTo(0));
  if (this->opi_extension_flag)
  {
    while (reader.more_rbsp_data())
    {
      this->opi_extension_data_flag = reader.readFlag("opi_extension_data_flag");
    }
  }
  this->rbsp_trailing_bits_instance.parse(reader);
}

} // namespace parser::vvc
