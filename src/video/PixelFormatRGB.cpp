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

#include "PixelFormatRGB.h"

// Activate this if you want to know when which buffer is loaded/converted to image and so on.
#define RGBPIXELFORMAT_DEBUG 0
#if RGBPIXELFORMAT_DEBUG && !NDEBUG
#include <QDebug>
#define DEBUG_RGB_FORMAT qDebug
#else
#define DEBUG_RGB_FORMAT(fmt, ...) ((void)0)
#endif

namespace video::rgb
{

PixelFormatRGB::PixelFormatRGB(unsigned     bitsPerSample,
                               DataLayout   dataLayout,
                               ChannelOrder channelOrder,
                               AlphaMode    alphaMode,
                               Endianness   endianness)
    : bitsPerSample(bitsPerSample), dataLayout(dataLayout), channelOrder(channelOrder),
      alphaMode(alphaMode), endianness(endianness)
{
}

PixelFormatRGB::PixelFormatRGB(const std::string &name)
{
  if (name != "Unknown Pixel Format")
  {
    auto channelOrderString = name.substr(0, 3);
    if (name[0] == 'a' || name[0] == 'A')
    {
      this->alphaMode    = AlphaMode::First;
      channelOrderString = name.substr(1, 3);
    }
    else if (name[3] == 'a' || name[3] == 'A')
    {
      this->alphaMode    = AlphaMode::Last;
      channelOrderString = name.substr(0, 3);
    }
    auto order = ChannelOrderMapper.getValue(channelOrderString);
    if (order)
      this->channelOrder = *order;

    auto bitIdx = name.find("bit");
    if (bitIdx != std::string::npos)
      this->bitsPerSample = std::stoi(name.substr(bitIdx - 2, 2), nullptr);
    if (name.find("planar") != std::string::npos)
      this->dataLayout = DataLayout::Planar;
    if (this->bitsPerSample > 8 && name.find("BE") != std::string::npos)
      this->endianness = Endianness::Big;
  }
}

bool PixelFormatRGB::isValid() const
{
  return this->bitsPerSample >= 8 && this->bitsPerSample <= 16;
}

unsigned PixelFormatRGB::nrChannels() const
{
  return this->alphaMode != AlphaMode::None ? 4 : 3;
}

bool PixelFormatRGB::hasAlpha() const
{
  return this->alphaMode != AlphaMode::None;
}

std::string PixelFormatRGB::getName() const
{
  if (!this->isValid())
    return "Unknown Pixel Format";

  std::string name;
  if (this->alphaMode == AlphaMode::First)
    name += "A";
  name += ChannelOrderMapper.getName(this->channelOrder);
  if (this->alphaMode == AlphaMode::Last)
    name += "A";

  name += " " + std::to_string(this->bitsPerSample) + "bit";
  if (this->dataLayout == DataLayout::Planar)
    name += " planar";
  if (this->bitsPerSample > 8 && this->endianness == Endianness::Big)
    name += " BE";

  return name;
}

/* Get the number of bytes for a frame with this RGB format and the given size
 */
std::size_t PixelFormatRGB::bytesPerFrame(Size frameSize) const
{
  auto bpsValid = this->bitsPerSample >= 8 && this->bitsPerSample <= 16;
  if (!bpsValid || !frameSize.isValid())
    return 0;

  auto numSamples = std::size_t(frameSize.height) * std::size_t(frameSize.width);
  auto nrBytes    = numSamples * this->nrChannels() * ((this->bitsPerSample + 7) / 8);
  DEBUG_RGB_FORMAT("PixelFormatRGB::bytesPerFrame samples %d channels %d bytes %d",
                   int(numSamples),
                   this->nrChannels(),
                   nrBytes);
  return nrBytes;
}

int PixelFormatRGB::getComponentPosition(Channel channel) const
{
  if (channel == Channel::Alpha)
  {
    switch (this->alphaMode)
    {
    case AlphaMode::First:
      return 0;
    case AlphaMode::Last:
      return 3;
    default:
      return -1;
    }
  }

  auto rgbIdx = 0;
  if (channel == Channel::Red)
  {
    if (this->channelOrder == ChannelOrder::RGB || this->channelOrder == ChannelOrder::RBG)
      rgbIdx = 0;
    if (this->channelOrder == ChannelOrder::GRB || this->channelOrder == ChannelOrder::BRG)
      rgbIdx = 1;
    if (this->channelOrder == ChannelOrder::GBR || this->channelOrder == ChannelOrder::BGR)
      rgbIdx = 2;
  }
  else if (channel == Channel::Green)
  {
    if (this->channelOrder == ChannelOrder::GRB || this->channelOrder == ChannelOrder::GBR)
      rgbIdx = 0;
    if (this->channelOrder == ChannelOrder::RGB || this->channelOrder == ChannelOrder::BGR)
      rgbIdx = 1;
    if (this->channelOrder == ChannelOrder::RBG || this->channelOrder == ChannelOrder::BRG)
      rgbIdx = 2;
  }
  else if (channel == Channel::Blue)
  {
    if (this->channelOrder == ChannelOrder::BGR || this->channelOrder == ChannelOrder::BRG)
      rgbIdx = 0;
    if (this->channelOrder == ChannelOrder::RBG || this->channelOrder == ChannelOrder::GBR)
      rgbIdx = 1;
    if (this->channelOrder == ChannelOrder::RGB || this->channelOrder == ChannelOrder::GRB)
      rgbIdx = 2;
  }

  if (this->alphaMode == AlphaMode::First)
    return rgbIdx + 1;
  return rgbIdx;
}

} // namespace video::rgb