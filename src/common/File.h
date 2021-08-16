/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <QString>
#include <atomic>
#include <chrono>
#include <QByteArray>

using Time = std::chrono::time_point<std::chrono::steady_clock>;
struct File
{
  QString     pathOrURL;
  QByteArray  fileData;
  Time        start{};
  Time        end{};
  std::size_t nrBytes{};
  bool        isLocalFile{};

  using Percent = double;
  std::atomic<Percent> downloadProgress{0.0};
};
