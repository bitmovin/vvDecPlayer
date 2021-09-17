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

#include "nal_unit_header.h"

#include <memory>

namespace parser::vvc
{

class NalRBSP
{
public:
  NalRBSP()          = default;
  virtual ~NalRBSP() = default;
};

class NalUnitVVC
{
public:
  NalUnitVVC(int nalIdx, std::optional<pairUint64> filePosStartEnd)
      : nalIdx(nalIdx), filePosStartEnd(filePosStartEnd)
  {
  }

  int nalIdx{};
  // Pointer to the first byte of the start code of the NAL unit (if known)
  std::optional<pairUint64> filePosStartEnd;

  nal_unit_header          header;
  std::shared_ptr<NalRBSP> rbsp;

  ByteVector rawData;
};

using NalMap = std::map<unsigned, std::shared_ptr<vvc::NalUnitVVC>>;

} // namespace parser::vvc