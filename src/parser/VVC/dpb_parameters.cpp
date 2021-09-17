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

#include "dpb_parameters.h"

namespace parser::vvc
{

using namespace parser::reader;

void dpb_parameters::parse(SubByteReaderLogging &reader,
                           unsigned         MaxSubLayersMinus1,
                           bool             subLayerInfoFlag)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "dpb_parameters");

  for (unsigned i = (subLayerInfoFlag ? 0 : MaxSubLayersMinus1); i <= MaxSubLayersMinus1; i++)
  {
    this->dpb_max_dec_pic_buffering_minus1.push_back(
        reader.readUEV("dpb_max_dec_pic_buffering_minus1"));
    this->dpb_max_num_reorder_pics.push_back(reader.readUEV("dpb_max_num_reorder_pics"));
    this->dpb_max_latency_increase_plus1.push_back(
        reader.readUEV("dpb_max_latency_increase_plus1"));
  }
}

} // namespace parser::vvc
