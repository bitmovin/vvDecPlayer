/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <common/typedef.h>

#include <optional>
#include <QByteArray>

std::optional<std::size_t> findNextNalInData(const QByteArray &data, std::size_t start);
ByteVector convertToByteVector(QByteArray data);
