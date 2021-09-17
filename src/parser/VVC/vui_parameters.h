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

#include "parser/common/CodingEnum.h"
#include "parser/common/SubByteReaderLogging.h"

namespace parser::vvc
{

// Table 7 in T-REC-H.273-201612
static CodingEnum<Ratio> sampleAspectRatioCoding({{0, {0, 0}, "Unspecified"},
                                                  {1, {1, 1}, "1:1 (Square)"},
                                                  {2, {12, 11}, "12:11"},
                                                  {3, {10, 11}, "10:11"},
                                                  {4, {16, 11}, "16:11"},
                                                  {5, {40, 33}, "40:33"},
                                                  {6, {24, 11}, "24:11"},
                                                  {7, {20, 11}, "20:11"},
                                                  {8, {32, 11}, "32:11"},
                                                  {9, {80, 33}, "80:33"},
                                                  {10, {18, 11}, "18:11"},
                                                  {11, {15, 11}, "15:11"},
                                                  {12, {64, 33}, "64:33"},
                                                  {13, {160, 99}, "160:99"},
                                                  {14, {4, 3}, "4:3"},
                                                  {15, {3, 2}, "3:2"},
                                                  {16, {2, 1}, "2:1"},
                                                  {255, {0, 0}, "SarWidth:SarHeight (Custom)"}},
                                                 {0, 0});

class vui_parameters
{
public:
  vui_parameters()  = default;
  ~vui_parameters() = default;
  void parse(reader::SubByteReaderLogging &reader, unsigned payloadSize);

  bool vui_progressive_source_flag{};
  bool vui_interlaced_source_flag{};
  bool vui_non_packed_constraint_flag{};
  bool vui_non_projected_constraint_flag{};
  bool vui_aspect_ratio_info_present_flag{};

  bool     vui_aspect_ratio_constant_flag{};
  unsigned vui_aspect_ratio_idc{};
  unsigned vui_sar_width{};
  unsigned vui_sar_height{};

  bool     vui_overscan_info_present_flag{};
  bool     vui_overscan_appropriate_flag{};
  bool     vui_colour_description_present_flag{};
  unsigned vui_colour_primaries{};
  unsigned vui_transfer_characteristics{};
  unsigned vui_matrix_coeffs{};
  bool     vui_full_range_flag{};
  bool     vui_chroma_loc_info_present_flag{};

  uint64_t vui_chroma_sample_loc_type_frame{};
  uint64_t vui_chroma_sample_loc_type_top_field{};
  uint64_t vui_chroma_sample_loc_type_bottom_field{};
};

} // namespace parser::vvc
