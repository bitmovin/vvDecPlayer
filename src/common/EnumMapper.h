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
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>

/* This class implement mapping of "enum class" values to and from names (string).
 */
template <typename T> class EnumMapper
{
public:
  struct Entry
  {
    Entry(T value, std::string name) : value(value), name(name) {}
    Entry(T value, std::string name, std::string text) : value(value), name(name), text(text) {}
    T           value;
    std::string name;
    std::string text;
  };

  using EntryVector = std::vector<Entry>;

  EnumMapper() = default;
  EnumMapper(const EntryVector &entryVector) : entryVector(entryVector){};

  std::optional<T> getValue(std::string name, bool isText = false) const
  {
    for (const auto &entry : this->entryVector)
      if ((!isText && entry.name == name) || (isText && entry.text == name))
        return entry.value;
    return {};
  }

  std::string getName(T value) const
  {
    for (const auto &entry : this->entryVector)
      if (entry.value == value)
        return entry.name;
    throw std::logic_error(
        "The given type T was not registered in the mapper. All possible enums must be mapped.");
  }

  std::string getText(T value) const
  {
    for (const auto &entry : this->entryVector)
      if (entry.value == value)
        return entry.text;
    throw std::logic_error(
        "The given type T was not registered in the mapper. All possible enums must be mapped.");
  }

  size_t indexOf(T value) const
  {
    for (size_t i = 0; i < this->entryVector.size(); i++)
      if (this->entryVector.at(i).value == value)
        return i;
    throw std::logic_error(
        "The given type T was not registered in the mapper. All possible enums must be mapped.");
  }

  std::optional<T> at(size_t index) const
  {
    if (index >= this->entryVector.size())
      return {};
    return this->entryVector.at(index).value;
  }

  std::vector<T> getEnums() const
  {
    std::vector<T> m;
    for (const auto &entry : this->entryVector)
      m.push_back(entry.value);
    return m;
  }

  std::vector<std::string> getNames() const
  {
    std::vector<std::string> l;
    for (const auto &entry : this->entryVector)
      l.push_back(entry.name);
    return l;
  }

  std::vector<std::string> getTextEntries() const
  {
    std::vector<std::string> l;
    for (const auto &entry : this->entryVector)
      l.push_back(entry.text);
    return l;
  }

  size_t size() const { return this->entryVector.size(); }

  const EntryVector &entries() const { return this->entryVector; }

private:
  EntryVector entryVector;
};
