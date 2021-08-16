/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

#include "typedef.h"

QImage::Format pixmapImageFormat()
{
  static auto const format = QPixmap(1, 1).toImage().format();
  Q_ASSERT(format != QImage::Format_Invalid);
  return format;
}
