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

#include <algorithm>
#include <string>
#include <tuple>

#include "common/typedef.h"

namespace parser
{

/* This class provides the ability to read a byte array bit wise. Reading of ue(v) symbols is also
 * supported. This class can "read out" the emulation prevention bytes. This is enabled by default
 * but can be disabled if needed.
 */
class SubByteReader
{
public:
  SubByteReader() = default;
  SubByteReader(const ByteVector &inArr, size_t inArrOffset = 0);

  [[nodiscard]] bool more_rbsp_data() const;
  [[nodiscard]] bool byte_aligned() const;

  [[nodiscard]] bool payload_extension_present() const;
  [[nodiscard]] bool canReadBits(unsigned nrBits) const;

  [[nodiscard]] size_t nrBitsRead() const;
  [[nodiscard]] size_t nrBytesRead() const;
  [[nodiscard]] size_t nrBytesLeft() const;

  [[nodiscard]] ByteVector peekBytes(unsigned nrBytes) const;

  void disableEmulationPrevention() { skipEmulationPrevention = false; }

protected:
  std::tuple<uint64_t, std::string>   readBits(size_t nrBits);
  std::tuple<ByteVector, std::string> readBytes(size_t nrBytes);

  std::tuple<uint64_t, std::string> readUE_V();
  std::tuple<int64_t, std::string>  readSE_V();
  std::tuple<uint64_t, std::string> readLEB128();
  std::tuple<uint64_t, std::string> readUVLC();
  std::tuple<uint64_t, std::string> readNS(uint64_t maxVal);
  std::tuple<int64_t, std::string>  readSU(unsigned nrBits);

  ByteVector byteVector;

  bool skipEmulationPrevention{true};

  // Move to the next byte and look for an emulation prevention 3 byte. Remove it (skip it) if
  // found. This function is just used by the internal reading functions.
  bool gotoNextByte();

  size_t posInBufferBytes{0};    // The byte position in the buffer
  size_t posInBufferBits{0};     // The sub byte (bit) position in the buffer (0...7)
  size_t numEmuPrevZeroBytes{0}; // The number of emulation prevention three bytes that were found
  size_t initialPosInBuffer{0};  // The position that was given when creating the sub reader
};

} // namespace parser