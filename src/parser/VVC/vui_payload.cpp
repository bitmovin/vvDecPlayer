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

#include "vui_payload.h"

namespace parser::vvc
{

using namespace reader;

void vui_payload::parse(reader::SubByteReaderLogging &reader, unsigned payloadSize)
{
  vui.parse(reader, payloadSize);
  auto more_data_in_payload = !reader.byte_aligned() || reader.nrBytesRead() != payloadSize;
  if (more_data_in_payload)
  {
    size_t nrBits = 8 * payloadSize - reader.nrBitsRead();
    for (; nrBits >= 8; nrBits -= 8)
    {
      reader.readBits("vui_reserved_payload_extension_data_byte", 8);
    }
    reader.readBits("vui_reserved_payload_extension_data_byte", int(nrBits));

    reader.readBits("vui_payload_bit_equal_to_one", 1, Options().withCheckEqualTo(1));
    while (!reader.byte_aligned())
    {
      reader.readBits("vui_payload_bit_equal_to_zero", 1, Options().withCheckEqualTo(0));
    }
  }
}

} // namespace parser::vvc
