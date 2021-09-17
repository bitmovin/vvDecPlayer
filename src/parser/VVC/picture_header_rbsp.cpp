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

#include "picture_header_rbsp.h"

#include "pic_parameter_set_rbsp.h"
#include "seq_parameter_set_rbsp.h"
#include "slice_layer_rbsp.h"

namespace parser::vvc
{

using namespace parser::reader;

void picture_header_rbsp::parse(SubByteReaderLogging &                 reader,
                                VPSMap &                          vpsMap,
                                SPSMap &                          spsMap,
                                PPSMap &                          ppsMap,
                                std::shared_ptr<slice_layer_rbsp> sl)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "picture_header_rbsp");

  this->picture_header_structure_instance = std::make_shared<picture_header_structure>();
  this->picture_header_structure_instance->parse(reader, vpsMap, spsMap, ppsMap, sl);
  this->rbsp_trailing_bits_instance.parse(reader);
}

} // namespace parser::vvc
