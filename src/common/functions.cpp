/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "functions.h"

std::optional<std::size_t> findNextNalInData(const QByteArray &data, std::size_t start)
{
  auto START_CODE = QByteArrayLiteral("\x00\x00\x01");
  
  if (start >= size_t(data.size()))
    return {};
  auto newStart = data.indexOf(START_CODE, start);
  if (newStart < 0)
    return {};
  if (newStart >= 1 && data.at(newStart - 1) == char(0x00))
    newStart -= 1;
  return newStart;
}

ByteVector convertToByteVector(QByteArray data)
{
  auto d = data.data();
  return ByteVector(d, d + data.size());
}
