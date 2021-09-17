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

#include "profile_tier_level.h"

namespace parser::vvc
{

using namespace parser::reader;

void profile_tier_level::parse(SubByteReaderLogging &reader,
                               bool             profileTierPresentFlag,
                               unsigned         MaxNumSubLayersMinus1)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "profile_tier_level");

  if (profileTierPresentFlag)
  {
    this->general_profile_idc = reader.readBits("general_profile_idc", 7);
    this->general_tier_flag   = reader.readFlag("general_tier_flag");
  }
  this->general_level_idc              = reader.readBits("general_level_idc", 8);
  this->ptl_frame_only_constraint_flag = reader.readFlag("ptl_frame_only_constraint_flag");
  this->ptl_multilayer_enabled_flag    = reader.readFlag("ptl_multilayer_enabled_flag");
  if (profileTierPresentFlag)
  {
    this->general_constraints_info_instance.parse(reader);
  }
  for (int i = MaxNumSubLayersMinus1 - 1; i >= 0; i--)
  {
    this->ptl_sublayer_level_present_flag.push_back(
        reader.readFlag("ptl_sublayer_level_present_flag"));
  }
  while (!reader.byte_aligned())
  {
    this->ptl_reserved_zero_bit = reader.readFlag("ptl_reserved_zero_bit");
  }
  for (int i = MaxNumSubLayersMinus1 - 1; i >= 0; i--)
  {
    if (this->ptl_sublayer_level_present_flag[i])
    {
      this->sublayer_level_idc.push_back(reader.readBits("sublayer_level_idc", 8));
    }
  }
  if (profileTierPresentFlag)
  {
    this->ptl_num_sub_profiles = reader.readBits("ptl_num_sub_profiles", 8);
    for (unsigned i = 0; i < this->ptl_num_sub_profiles; i++)
    {
      this->general_sub_profile_idc.push_back(reader.readBits("general_sub_profile_idc", 32));
    }
  }
}

} // namespace parser::vvc
