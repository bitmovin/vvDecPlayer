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

#include "general_constraints_info.h"
#include "parser/common/SubByteReaderLogging.h"


namespace parser::vvc
{

class profile_tier_level
{
public:
  profile_tier_level()  = default;
  ~profile_tier_level() = default;
  void parse(reader::SubByteReaderLogging &reader,
             bool                     profileTierPresentFlag,
             unsigned                 MaxNumSubLayersMinus1);

  unsigned                 general_profile_idc{};
  bool                     general_tier_flag{};
  unsigned                 general_level_idc{};
  bool                     ptl_frame_only_constraint_flag{};
  bool                     ptl_multilayer_enabled_flag{};
  general_constraints_info general_constraints_info_instance;
  std::vector<bool>        ptl_sublayer_level_present_flag{};
  bool                     ptl_reserved_zero_bit{};
  std::vector<unsigned>    sublayer_level_idc{};
  unsigned                 ptl_num_sub_profiles{};
  std::vector<unsigned>    general_sub_profile_idc{};
};

} // namespace parser::vvc
