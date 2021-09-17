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

#include "common/typedef.h"
#include <map>
#include <string>
#include <vector>

namespace parser
{

template <typename T> std::string formatArray(std::string variableName, T idx)
{
  return variableName + "[" + std::to_string(idx) + "]";
}

template <typename T1, typename T2>
std::string formatArray(std::string variableName, T1 idx1, T2 idx2)
{
  return variableName + "[" + std::to_string(idx1) + "][" + std::to_string(idx2) + "]";
}

template <typename T1, typename T2, typename T3>
std::string formatArray(std::string variableName, T1 idx1, T2 idx2, T3 idx3)
{
  return variableName + "[" + std::to_string(idx1) + "][" + std::to_string(idx2) + "][" +
         std::to_string(idx3) + "]";
}

} // namespace parser
