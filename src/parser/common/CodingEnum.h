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

#include <map>
#include <string>
#include <vector>

namespace parser
{

template <typename T> class CodingEnum
{
public:
  struct Entry
  {
    Entry(unsigned code, T value, std::string name, std::string meaning = "")
        : code(code), value(value), name(name), meaning(meaning)
    {
    }
    unsigned    code;
    T           value;
    std::string name;
    std::string meaning;
  };

  using EntryVector = std::vector<Entry>;

  CodingEnum() = default;
  CodingEnum(const EntryVector &entryVector, const T unknown)
      : entryVector(entryVector), unknown(unknown){};

  T getValue(unsigned code) const
  {
    for (const auto &entry : this->entryVector)
      if (entry.code == code)
        return entry.value;
    return this->unknown;
  }

  unsigned getCode(T value) const
  {
    for (const auto &entry : this->entryVector)
      if (entry.value == value)
        return entry.code;
    return {};
  }

  std::map<int, std::string> getMeaningMap() const
  {
    std::map<int, std::string> m;
    for (const auto &entry : this->entryVector)
    {
      if (entry.meaning.empty())
        m[int(entry.code)] = entry.name;
      else
        m[int(entry.code)] = entry.meaning;
    }
    return m;
  }

  std::string getMeaning(T value) const
  {
    for (const auto &entry : this->entryVector)
      if (entry.value == value)
        return entry.meaning;
    return {};
  }

private:
  EntryVector entryVector;
  T           unknown;
};

} // namespace parser
