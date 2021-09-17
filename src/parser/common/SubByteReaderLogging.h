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
#include <stack>

#include "SubByteReader.h"
#include "SubByteReaderLoggingOptions.h"
#include "TreeItem.h"
#include "common/typedef.h"

namespace parser::reader
{

typedef std::string (*meaning_callback_function)(unsigned int);

class SubByteReaderLoggingSubLevel;

// This is a wrapper around the sub_byte_reader that adds the functionality to log the read symbold
// to TreeItems
class SubByteReaderLogging : public SubByteReader
{
public:
  SubByteReaderLogging() = default;
  SubByteReaderLogging(SubByteReader &           reader,
                       std::shared_ptr<TreeItem> item,
                       std::string               new_sub_item_name = "");
  SubByteReaderLogging(const ByteVector &        inArr,
                       std::shared_ptr<TreeItem> item,
                       std::string               new_sub_item_name = "",
                       size_t                    inOffset          = 0);

  // DEPRECATED. This is just for backwards compatibility and will be removed once
  // everything is using std types.
  static ByteVector convertToByteVector(QByteArray data);
  static QByteArray convertToQByteArray(ByteVector data);

  uint64_t readBits(const std::string &symbolName, size_t numBits, const Options &options = {});
  bool     readFlag(const std::string &symbolName, const Options &options = {});
  uint64_t readUEV(const std::string &symbolName, const Options &options = {});
  int64_t  readSEV(const std::string &symbolName, const Options &options = {});
  uint64_t readLEB128(const std::string &symbolName, const Options &options = {});
  uint64_t readNS(const std::string &symbolName, uint64_t maxVal, const Options &options = {});
  int64_t  readSU(const std::string &symbolName, unsigned nrBits, const Options &options = {});

  ByteVector readBytes(const std::string &symbolName, size_t nrBytes, const Options &options = {});

  void
  logCalculatedValue(const std::string &symbolName, int64_t value, const Options &options = {});
  void logArbitrary(const std::string &symbolName,
                    const std::string &value   = {},
                    const std::string &coding  = {},
                    const std::string &code    = {},
                    const std::string &meaning = {});

  void stashAndReplaceCurrentTreeItem(std::shared_ptr<TreeItem> newItem);
  void popTreeItem();

  [[nodiscard]] std::shared_ptr<TreeItem> getCurrentItemTree() { return this->currentTreeLevel; }

private:
  friend class SubByteReaderLoggingSubLevel;
  void addLogSubLevel(std::string name);
  void removeLogSubLevel();

  void logExceptionAndThrowError [[noreturn]] (const std::exception &ex, const std::string &when);

  std::stack<std::shared_ptr<TreeItem>> itemHierarchy;
  std::shared_ptr<TreeItem>             currentTreeLevel{};
  std::shared_ptr<TreeItem>             stashedTreeItem{};
};

// A simple wrapper for SubByteReaderLogging->addLogSubLevel /
// SubByteReaderLogging->removeLogSubLevel
class SubByteReaderLoggingSubLevel
{
public:
  SubByteReaderLoggingSubLevel() = default;
  SubByteReaderLoggingSubLevel(SubByteReaderLogging &reader, std::string name);
  ~SubByteReaderLoggingSubLevel();

private:
  SubByteReaderLogging *r{};
};

} // namespace parser::reader