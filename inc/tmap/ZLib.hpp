/****************************************************************************

    MIT License

    Copyright (c) 2020 Aria Janke

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
    SOFTWARE.

*****************************************************************************/

#pragma once

#include <vector>

namespace ZLib {

using UInt8      = unsigned char;
using ByteBuffer = std::vector<UInt8>;

ByteBuffer decompress(const ByteBuffer & src_data);

ByteBuffer decompress(const ByteBuffer & src_data, ByteBuffer & cache_data);

enum class CompressionLevel {
    k_default_compression = -1,
    k_no_compression      =  0,
    k_best_speed          =  1,
    k_medium_compression  =  5,
    k_best_compression    =  9
};

ByteBuffer compress(const ByteBuffer & src, CompressionLevel level);

ByteBuffer compress
    (const ByteBuffer & src, CompressionLevel level, ByteBuffer & cache_out);

// useful util functions

ByteBuffer dump_file_to_buffer(const char * filename);

void dump_buffer_to_file(const char * filename, const ByteBuffer & buff);

} // end of ZLib namespace
