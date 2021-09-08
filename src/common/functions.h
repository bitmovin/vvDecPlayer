/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <optional>
#include <QByteArray>
#include <common/typedef.h>

std::optional<std::size_t> findNextNalInData(const QByteArray &data, std::size_t start);
ByteVector convertToByteVector(QByteArray data);
