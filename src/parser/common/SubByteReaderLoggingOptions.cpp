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

#include "SubByteReaderLoggingOptions.h"

namespace parser::reader
{

namespace
{

struct CheckEqualTo : Check
{
public:
  CheckEqualTo() = delete;
  CheckEqualTo(int64_t value, std::string errorIfFail) : Check(errorIfFail), value(value) {}
  CheckResult checkValue(int64_t value) const override;

private:
  int64_t value;
};

struct CheckGreater : Check
{
public:
  CheckGreater() = delete;
  CheckGreater(int64_t value, bool inclusive, std::string errorIfFail)
      : Check(errorIfFail), value(value), inclusive(inclusive)
  {
  }
  CheckResult checkValue(int64_t value) const override;

private:
  int64_t value;
  bool    inclusive;
};

struct CheckSmaller : Check
{
public:
  CheckSmaller() = delete;
  CheckSmaller(int64_t value, bool inclusive, std::string errorIfFail)
      : Check(errorIfFail), value(value), inclusive(inclusive)
  {
  }
  CheckResult checkValue(int64_t value) const override;

private:
  int64_t value;
  bool    inclusive;
};

struct CheckRange : Check
{
public:
  CheckRange() = delete;
  CheckRange(Range<int64_t> range, bool inclusive, std::string errorIfFail)
      : Check(errorIfFail), range(range), inclusive(inclusive)
  {
  }
  CheckResult checkValue(int64_t value) const override;

private:
  Range<int64_t> range;
  bool           inclusive;
};

} // namespace

CheckResult CheckEqualTo::checkValue(int64_t value) const
{
  if (value != this->value)
  {
    if (!this->errorIfFail.empty())
      return CheckResult({this->errorIfFail});
    return CheckResult({"Value should be equal to " + std::to_string(this->value)});
  }
  return {};
}

CheckResult CheckGreater::checkValue(int64_t value) const
{
  auto checkFailed =
      (this->inclusive && value < this->value) || (!this->inclusive && value <= this->value);
  if (checkFailed)
  {
    if (!this->errorIfFail.empty())
      return CheckResult({this->errorIfFail});
    return CheckResult({"Value should be greater then " + std::to_string(this->value) +
                        (this->inclusive ? " inclusive." : " exclusive.")});
  }
  return {};
}

CheckResult CheckSmaller::checkValue(int64_t value) const
{
  auto checkFailed =
      (this->inclusive && value > this->value) || (!this->inclusive && value >= this->value);
  if (checkFailed)
  {
    if (!this->errorIfFail.empty())
      return CheckResult({this->errorIfFail});
    return CheckResult({"Value should be smaller then " + std::to_string(this->value) +
                        (this->inclusive ? " inclusive." : " exclusive.")});
  }
  return {};
}

CheckResult CheckRange::checkValue(int64_t value) const
{
  auto checkFailed = (this->inclusive && (value < this->range.min || value > this->range.max)) ||
                     (!this->inclusive && (value <= this->range.min || value >= this->range.max));
  if (checkFailed)
  {
    if (!this->errorIfFail.empty())
      return CheckResult({this->errorIfFail});
    return CheckResult({"Value should be in the range of " + std::to_string(this->range.min) +
                        " to " + std::to_string(this->range.max) +
                        (this->inclusive ? " inclusive." : " exclusive.")});
  }
  return {};
}

Options &&Options::withMeaning(const std::string &meaningString)
{
  this->meaningString = meaningString;
  return std::move(*this);
}

Options &&Options::withMeaningMap(const MeaningMap &meaningMap)
{
  this->meaningMap = meaningMap;
  return std::move(*this);
}

Options &&Options::withMeaningVector(const std::vector<std::string> &meaningVector)
{
  // This is just a conveniance function. We still save the data in the map starting with an index
  // of 0
  for (unsigned i = 0; i < meaningVector.size(); i++)
    this->meaningMap[i] = meaningVector[i];
  return std::move(*this);
}

Options &&Options::withMeaningFunction(const std::function<std::string(int64_t)> &meaningFunction)
{
  this->meaningFunction = meaningFunction;
  return std::move(*this);
}

Options &&Options::withCheckEqualTo(int64_t value, const std::string &errorIfFail)
{
  this->checkList.emplace_back(std::make_unique<CheckEqualTo>(value, errorIfFail));
  return std::move(*this);
}

Options &&Options::withCheckGreater(int64_t value, bool inclusive, const std::string &errorIfFail)
{
  this->checkList.emplace_back(std::make_unique<CheckGreater>(value, inclusive, errorIfFail));
  return std::move(*this);
}

Options &&Options::withCheckSmaller(int64_t value, bool inclusive, const std::string &errorIfFail)
{
  this->checkList.emplace_back(std::make_unique<CheckSmaller>(value, inclusive, errorIfFail));
  return std::move(*this);
}

Options &&
Options::withCheckRange(Range<int64_t> range, bool inclusive, const std::string &errorIfFail)
{
  this->checkList.emplace_back(std::make_unique<CheckRange>(range, inclusive, errorIfFail));
  return std::move(*this);
}

Options &&Options::withLoggingDisabled()
{
  this->loggingDisabled = true;
  return std::move(*this);
}

} // namespace parser::reader