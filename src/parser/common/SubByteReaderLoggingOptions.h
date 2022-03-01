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

#include "common/Typedef.h"

#include <functional>
#include <memory>
#include <optional>

namespace parser::reader
{

using MeaningMap = std::map<int, std::string>;

struct CheckResult
{
  explicit    operator bool() const { return this->errorMessage.empty(); }
  std::string errorMessage;
};

class Check
{
public:
  Check() = default;
  Check(std::string errorIfFail) : errorIfFail(errorIfFail){};
  virtual ~Check() = default;

  virtual CheckResult checkValue(int64_t value) const = 0;

  std::string errorIfFail;
};

struct Options
{
  Options() = default;

  [[nodiscard]] Options &&withMeaning(const std::string &meaningString);
  [[nodiscard]] Options &&withMeaningMap(const MeaningMap &meaningMap);
  [[nodiscard]] Options &&withMeaningVector(const std::vector<std::string> &meaningVector);
  [[nodiscard]] Options &&
  withMeaningFunction(const std::function<std::string(int64_t)> &meaningFunction);
  [[nodiscard]] Options &&withCheckEqualTo(int64_t value, const std::string &errorIfFail = {});
  [[nodiscard]] Options &&
  withCheckGreater(int64_t value, bool inclusive = true, const std::string &errorIfFail = {});
  [[nodiscard]] Options &&
  withCheckSmaller(int64_t value, bool inclusive = true, const std::string &errorIfFail = {});
  [[nodiscard]] Options &&
  withCheckRange(Range<int64_t> range, bool inclusive = true, const std::string &errorIfFail = {});
  [[nodiscard]] Options &&withLoggingDisabled();

  std::string                         meaningString;
  std::map<int, std::string>          meaningMap;
  std::function<std::string(int64_t)> meaningFunction;
  std::vector<std::unique_ptr<Check>> checkList;
  bool                                loggingDisabled{false};
};

} // namespace parser::reader