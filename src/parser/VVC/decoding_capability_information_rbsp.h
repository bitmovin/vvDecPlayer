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
#include "profile_tier_level.h"
#include "rbsp_trailing_bits.h"

namespace parser::vvc
{

class decoding_capability_information_rbsp : public NalRBSP
{
public:
  decoding_capability_information_rbsp()  = default;
  ~decoding_capability_information_rbsp() = default;
  void parse(reader::SubByteReaderLogging &reader);

  unsigned           dci_reserved_zero_4bits{};
  unsigned           dci_num_ptls_minus1{};
  profile_tier_level profile_tier_level_instance;
  bool               dci_extension_flag{};
  bool               dci_extension_data_flag{};
  rbsp_trailing_bits rbsp_trailing_bits_instance;
};

} // namespace parser::vvc
