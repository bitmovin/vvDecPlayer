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

#include "parser/common/SubByteReaderLogging.h"

namespace parser::vvc
{

class general_timing_hrd_parameters;

class sublayer_hrd_parameters
{
public:
  sublayer_hrd_parameters()  = default;
  ~sublayer_hrd_parameters() = default;
  void parse(reader::SubByteReaderLogging &      reader,
             unsigned                       subLayerId,
             general_timing_hrd_parameters *general_hrd);

  umap_2d<unsigned> bit_rate_value_minus1{};
  umap_2d<unsigned> cpb_size_value_minus1{};
  umap_2d<unsigned> cpb_size_du_value_minus1{};
  umap_2d<unsigned> bit_rate_du_value_minus1{};
  umap_2d<bool>     cbr_flag{};
};

} // namespace parser::vvc
