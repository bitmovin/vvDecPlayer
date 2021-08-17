/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QImage>
#include <QPixmap>

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
