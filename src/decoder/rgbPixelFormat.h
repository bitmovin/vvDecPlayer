/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include "common/typedef.h"
#include <string>

namespace RGB_Internals
{

// This class defines a specific RGB format with all properties like order of R/G/B, bitsPerValue,
// planarity...
class rgbPixelFormat
{
public:
  // The default constructor (will create an "Unknown Pixel Format")
  rgbPixelFormat() {}
  rgbPixelFormat(const std::string &name);
  rgbPixelFormat(
      int bitsPerValue, bool planar, int posR = 0, int posG = 1, int posB = 2, int posA = -1);
  bool operator==(const rgbPixelFormat &a) const
  {
    return getName() == a.getName();
  } // Comparing names should be enough since you are not supposed to create your own rgbPixelFormat
    // instances anyways.
  bool operator!=(const rgbPixelFormat &a) const { return getName() != a.getName(); }
  bool operator==(const std::string &a) const { return getName() == a; }
  bool operator!=(const std::string &a) const { return getName() != a; }
  bool isValid() const;
  int  nrChannels() const { return posA == -1 ? 3 : 4; }
  bool hasAlphaChannel() const { return posA != -1; }
  // Get a name representation of this item (this will be unique for the set parameters)
  std::string getName() const;
  // Get/Set the RGB format from string (accepted string are: "RGB", "BGR", ...)
  std::string getRGBFormatString() const;
  void        setRGBFormatFromString(const std::string &sFormat);
  // Get the number of bytes for a frame with this rgbPixelFormat and the given size
  int64_t bytesPerFrame(Size frameSize) const;
  int     getComponentPosition(unsigned channel) const;

  // The order of each component (E.g. for GBR this is posR=2,posG=0,posB=1)
  int  posR{0};
  int  posG{1};
  int  posB{2};
  int  posA{-1}; // The position of alpha can be -1 (no alpha channel)
  int  bitsPerValue{-1};
  bool planar{false};
};

} // namespace RGB_Internals
