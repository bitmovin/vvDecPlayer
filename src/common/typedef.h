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

#include <QImage>
#include <QPixmap>
#include <sstream>

#ifdef Q_OS_MAC
const bool is_Q_OS_MAC = true;
#else
const bool is_Q_OS_MAC = false;
#endif

#ifdef Q_OS_WIN
const bool is_Q_OS_WIN = true;
#else
const bool is_Q_OS_WIN = false;
#endif

#ifdef Q_OS_LINUX
const bool is_Q_OS_LINUX = true;
#else
const bool is_Q_OS_LINUX = false;
#endif

struct Size
{
  Size(unsigned width, unsigned height) : width(width), height(height) {}
  Size(int w, int h)
  {
    if (w > 0)
      this->width = unsigned(w);
    if (h > 0)
      this->height = unsigned(h);
  }
  Size() = default;
  bool operator==(const Size &other) const
  {
    return this->width == other.width && this->height == other.height;
  }
  bool operator!=(const Size &other) const
  {
    return this->width != other.width || this->height != other.height;
  }
  bool     isValid() const { return this->width != 0 && this->height != 0; }
  unsigned width{};
  unsigned height{};
};

enum class RawFormat
{
  Invalid,
  YUV,
  RGB
};

struct Offset
{
  Offset(int x, int y) : x(x), y(y) {}
  Offset() = default;
  bool operator==(const Offset &other) const { return this->x == other.x && this->x == other.x; }
  bool operator!=(const Offset &other) const { return this->x != other.x || this->y != other.y; }
  int  x{};
  int  y{};
};

// An image format used internally by QPixmap. On a raster paint backend, the pixmap
// is backed by an image, and this returns the format of the internal QImage buffer.
// This will always return the same result as the platformImageFormat when the default
// raster backend is used.
// It is faster to call platformImageFormat instead. It will call this function as
// a fall back.
// This function is thread-safe.
QImage::Format pixmapImageFormat();

// The platform-specific screen-compatible image format. Using a QImage of this format
// is fast when drawing on a widget.
// This function is thread-safe.
inline QImage::Format platformImageFormat()
{
  // see https://code.woboq.org/qt5/qtbase/src/gui/image/qpixmap_raster.cpp.html#97
  // see
  // https://code.woboq.org/data/symbol.html?root=../qt5/&ref=_ZN21QRasterPlatformPixmap18systemOpaqueFormatEv
  if (is_Q_OS_MAC)
    // https://code.woboq.org/qt5/qtbase/src/plugins/platforms/cocoa/qcocoaintegration.mm.html#117
    // https://code.woboq.org/data/symbol.html?root=../qt5/&ref=_ZN12QCocoaScreen14updateGeometryEv
    // Qt Docs: The image is stored using a 32-bit RGB format (0xffRRGGBB).
    return QImage::Format_RGB32;
  if (is_Q_OS_WIN)
    // https://code.woboq.org/qt5/qtbase/src/plugins/platforms/windows/qwindowsscreen.cpp.html#59
    // https://code.woboq.org/data/symbol.html?root=../qt5/&ref=_ZN18QWindowsScreenDataC1Ev
    // Qt Docs:
    // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB), i.e. the red,
    // green, and blue channels are multiplied by the alpha component divided by 255. (If RR, GG, or
    // BB has a higher value than the alpha channel, the results are undefined.) Certain operations
    // (such as image composition using alpha blending) are faster using premultiplied ARGB32 than
    // with plain ARGB32.
    return QImage::Format_ARGB32_Premultiplied;
  // Fall back on Linux and other platforms.
  return pixmapImageFormat();
}

template <typename T> unsigned clipToUnsigned(T val)
{
  static_assert(std::is_signed<T>::value, "T must must be a signed type");
  if (val < 0)
    return 0;
  return unsigned(val);
}

struct SegmentData
{
  SegmentData(size_t bitrate) : bitrate(bitrate) {}
  size_t bitrate{};
  double progressPercent{};
};

typedef std::vector<unsigned char> ByteVector;

template <typename T> struct Range
{
  T min{};
  T max{};
};

struct Ratio
{
  int num{};
  int den{};
};

/// ---- Custom types
typedef std::pair<int, int>           IntPair;
typedef std::pair<uint64_t, uint64_t> pairUint64;

template <typename T> using umap_1d = std::map<unsigned, T>;
template <typename T> using umap_2d = std::map<unsigned, umap_1d<T>>;
template <typename T> using umap_3d = std::map<unsigned, umap_2d<T>>;

template <typename T> using vector   = std::vector<T>;
template <typename T> using vector2d = std::vector<vector<T>>;
template <typename T> using vector3d = std::vector<vector2d<T>>;
template <typename T> using vector4d = std::vector<vector3d<T>>;

template <typename T> std::string to_string(const std::pair<T, T> typePair)
{
  std::ostringstream ss;
  ss << "(" << typePair.first << ", " << typePair.second << ")";
  return ss.str();
}

#define DEFAULT_FRAMERATE 24;
