/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#pragma once

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
