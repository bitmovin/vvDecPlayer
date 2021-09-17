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

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// The tree item is used to feed the tree view.
class TreeItem : public std::enable_shared_from_this<TreeItem>
{
public:
  TreeItem()  = default;
  ~TreeItem() = default;

  void setProperties(std::string name    = {},
                     std::string value   = {},
                     std::string coding  = {},
                     std::string code    = {},
                     std::string meaning = {})
  {
    this->name    = name;
    this->value   = value;
    this->coding  = coding;
    this->code    = code;
    this->meaning = meaning;
  }

  void setError(bool isError = true) { this->error = isError; }
  bool isError() const { return this->error; }

  std::string getName(bool showStreamIndex) const
  {
    std::stringstream ss;
    if (showStreamIndex && this->streamIndex >= 0)
      ss << "Stream " << this->streamIndex << " - ";
    ss << this->name;
    return ss.str();
  }

  int getStreamIndex() const
  {
    if (this->streamIndex >= 0)
      return this->streamIndex;
    if (auto p = this->parent.lock())
      return p->getStreamIndex();
    return -1;
  }
  void setStreamIndex(int idx) { this->streamIndex = idx; }

  template <typename T>
  std::shared_ptr<TreeItem> createChildItem(std::string name    = {},
                                            T           value   = {},
                                            std::string coding  = {},
                                            std::string code    = {},
                                            std::string meaning = {},
                                            bool        isError = false)
  {
    auto newItem    = std::make_shared<TreeItem>();
    newItem->parent = this->weak_from_this();
    newItem->setProperties(name, std::to_string(value), coding, code, meaning);
    newItem->error = isError;
    this->childItems.push_back(newItem);
    return newItem;
  }

  std::shared_ptr<TreeItem> createChildItem(std::string name    = {},
                                            std::string value   = {},
                                            std::string coding  = {},
                                            std::string code    = {},
                                            std::string meaning = {},
                                            bool        isError = false)
  {
    auto newItem    = std::make_shared<TreeItem>();
    newItem->parent = this->weak_from_this();
    newItem->setProperties(name, value, coding, code, meaning);
    newItem->error = isError;
    this->childItems.push_back(newItem);
    return newItem;
  }

  size_t getNrChildItems() const { return this->childItems.size(); }

  std::string getData(unsigned idx) const
  {
    switch (idx)
    {
    case 0:
      return this->name;
    case 1:
      return this->value;
    case 2:
      return this->coding;
    case 3:
      return this->code;
    case 4:
      return this->meaning;
    default:
      return {};
    }
  }

  const std::shared_ptr<TreeItem> getChild(unsigned idx) const
  {
    if (idx < this->childItems.size())
      return this->childItems[idx];
    return {};
  }

  std::weak_ptr<TreeItem> getParentItem() const { return this->parent; }

  std::optional<size_t> getIndexOfChildItem(std::shared_ptr<TreeItem> child) const
  {
    for (size_t i = 0; i < this->childItems.size(); i++)
      if (this->childItems[i] == child)
        return i;

    std::optional<size_t> ret;
    return ret;
  }

private:
  std::vector<std::shared_ptr<TreeItem>> childItems;
  std::weak_ptr<TreeItem>                parent{};

  std::string name;
  std::string value;
  std::string coding;
  std::string code;
  std::string meaning;

  bool error{};
  // This is set for the first layer items in case of AVPackets
  int streamIndex{-1};
};
