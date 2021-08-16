/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <QString>
#include <QTemporaryFile>

using Time = std::chrono::time_point<std::chrono::steady_clock>;
struct File
{
  QString        pathOrURL;
  QTemporaryFile localFile;
  Time           start{};
  Time           end{};
  std::size_t    nrBytes{};
  bool           isLocalFile{};

  using Percent = double;
  std::atomic<Percent> downloadProgress{0.0};
};
