/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "rgbPixelFormat.h"

// Activate this if you want to know when which buffer is loaded/converted to image and so on.
#define RGBPIXELFORMAT_DEBUG 0
#if RGBPIXELFORMAT_DEBUG && !NDEBUG
#include <QDebug>
#define DEBUG_RGB_FORMAT qDebug
#else
#define DEBUG_RGB_FORMAT(fmt, ...) ((void)0)
#endif

namespace RGB_Internals
{

rgbPixelFormat::rgbPixelFormat(
    int bitsPerValue, bool planar, int posR, int posG, int posB, int posA)
    : posR(posR), posG(posG), posB(posB), posA(posA), bitsPerValue(bitsPerValue), planar(planar)
{
}

rgbPixelFormat::rgbPixelFormat(const std::string &name)
{
  if (name == "Unknown Pixel Format")
  {
    this->posR         = 0;
    this->posG         = 0;
    this->posB         = 0;
    this->posA         = -1;
    this->bitsPerValue = 0;
    this->planar       = false;
  }
  else
  {
    this->setRGBFormatFromString(name.substr(0, 4));
    auto bitIdx = name.find("bit");
    if (bitIdx != std::string::npos)
      this->bitsPerValue = std::stoi(name.substr(bitIdx - 2, 2), nullptr);
    this->planar = (name.find("planar") != std::string::npos);
  }
}

bool rgbPixelFormat::isValid() const
{
  if (bitsPerValue < 8 || bitsPerValue > 16)
    return false;

  if (posR == posG || posR == posB || posG == posB)
    return false;

  const auto hasAlpha = (posA != -1);

  if (hasAlpha && (posA == posR || posA == posG || posA == posB))
    return false;

  const auto valueLimit = hasAlpha ? 4 : 3;

  if (posR < 0 || posR >= valueLimit || posG < 0 || posG >= valueLimit || posB < 0 ||
      posB >= valueLimit)
    return false;

  if (hasAlpha && (posA < 0 || posA >= valueLimit))
    return false;

  return true;
}

std::string rgbPixelFormat::getName() const
{
  if (!this->isValid())
    return "Unknown Pixel Format";

  auto name = getRGBFormatString();
  name += " " + std::to_string(this->bitsPerValue) + "bit";
  if (planar)
    name += " planar";

  return name;
}

std::string rgbPixelFormat::getRGBFormatString() const
{
  std::string name;
  for (int i = 0; i < nrChannels(); i++)
  {
    if (posR == i)
      name.append("R");
    if (posG == i)
      name.append("G");
    if (posB == i)
      name.append("B");
    if (posA == i)
      name.append("A");
  }
  return name;
}

void rgbPixelFormat::setRGBFormatFromString(const std::string &format)
{
  auto n = format.length();
  if (n < 3)
    return;
  if (n > 4)
    n = 4;

  for (unsigned i = 0; i < n; i++)
  {
    if (format[i] == 'r' || format[i] == 'R')
      posR = i;
    else if (format[i] == 'g' || format[i] == 'G')
      posG = i;
    else if (format[i] == 'b' || format[i] == 'B')
      posB = i;
    else if (format[i] == 'a' || format[i] == 'A')
      posA = i;
  }
}

/* Get the number of bytes for a frame with this RGB format and the given size
 */
int64_t rgbPixelFormat::bytesPerFrame(Size frameSize) const
{
  if (bitsPerValue == 0 || !frameSize.isValid())
    return 0;

  int64_t numSamples = frameSize.height * frameSize.width;
  int64_t nrBytes    = numSamples * nrChannels() * ((bitsPerValue + 7) / 8);
  DEBUG_RGB_FORMAT("rgbPixelFormat::bytesPerFrame samples %d channels %d bytes %d",
                   int(numSamples),
                   nrChannels(),
                   nrBytes);
  return nrBytes;
}

int rgbPixelFormat::getComponentPosition(unsigned channel) const
{
  if (channel == 0)
    return this->posR;
  if (channel == 1)
    return this->posG;
  if (channel == 2)
    return this->posB;
  if (channel == 3)
    return this->posA;
  return -1;
}

} // namespace RGB_Internals