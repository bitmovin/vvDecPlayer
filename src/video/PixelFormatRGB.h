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

#include <common/EnumMapper.h>
#include <common/Typedef.h>

#include "PixelFormat.h"

#include <string>

namespace video::rgb
{

enum class Channel
{
  Red,
  Green,
  Blue,
  Alpha
};

struct rgba_t
{
  unsigned R{0}, G{0}, B{0}, A{0};

  unsigned &operator[](Channel channel)
  {
    if (channel == Channel::Red)
      return this->R;
    if (channel == Channel::Green)
      return this->G;
    if (channel == Channel::Blue)
      return this->B;
    if (channel == Channel::Alpha)
      return this->A;

    throw std::out_of_range("Unsupported channel for value access");
  }
};

enum class ChannelOrder
{
  RGB,
  RBG,
  GRB,
  GBR,
  BRG,
  BGR
};

enum class AlphaMode
{
  None,
  First,
  Last
};

const auto ChannelOrderMapper = EnumMapper<ChannelOrder>({{ChannelOrder::RGB, "RGB"},
                                                          {ChannelOrder::RBG, "RBG"},
                                                          {ChannelOrder::GRB, "GRB"},
                                                          {ChannelOrder::GBR, "GBR"},
                                                          {ChannelOrder::BRG, "BRG"},
                                                          {ChannelOrder::BGR, "BGR"}});

// This class defines a specific RGB format with all properties like order of R/G/B, bitsPerValue,
// planarity...
class PixelFormatRGB
{
public:
  // The default constructor (will create an "Unknown Pixel Format")
  PixelFormatRGB() = default;
  PixelFormatRGB(const std::string &name);
  PixelFormatRGB(unsigned     bitsPerSample,
                 DataLayout   dataLayout,
                 ChannelOrder channelOrder,
                 AlphaMode    alphaMode  = AlphaMode::None,
                 Endianness   endianness = Endianness::Little);

  bool        isValid() const;
  unsigned    nrChannels() const;
  bool        hasAlpha() const;
  std::string getName() const;

  unsigned     getBitsPerSample() const { return this->bitsPerSample; }
  DataLayout   getDataLayout() const { return this->dataLayout; }
  ChannelOrder getChannelOrder() const { return this->channelOrder; }
  Endianness   getEndianess() const { return this->endianness; }

  void setBitsPerSample(unsigned bitsPerSample) { this->bitsPerSample = bitsPerSample; }
  void setDataLayout(DataLayout dataLayout) { this->dataLayout = dataLayout; }

  std::size_t bytesPerFrame(Size frameSize) const;
  int         getComponentPosition(Channel channel) const;

  bool operator==(const PixelFormatRGB &a) const { return getName() == a.getName(); }
  bool operator!=(const PixelFormatRGB &a) const { return getName() != a.getName(); }
  bool operator==(const std::string &a) const { return getName() == a; }
  bool operator!=(const std::string &a) const { return getName() != a; }

private:
  unsigned     bitsPerSample{0};
  DataLayout   dataLayout{DataLayout::Packed};
  ChannelOrder channelOrder{ChannelOrder::RGB};
  AlphaMode    alphaMode{AlphaMode::None};
  Endianness   endianness{Endianness::Little};
};

} // namespace video::rgb
