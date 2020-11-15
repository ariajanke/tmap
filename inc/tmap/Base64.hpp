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
#include <string>

/** Base64 codec functions, provides a simple interface for reading and
 *  writing in base64.
 *
 *  We've decided to move to STL to represent binary data. Vectors are
 *  gaurenteed to be contiguous. Raw data can be easily obtained using STL's
 *  vector facilities.
 *
 */
namespace Base64 {

using UInt8 = unsigned char;
using ByteBuffer = std::vector<UInt8>;

/** @class Base64_encode
 *  Encodes the given binary information into a base64 string (with "+/").
 *  note: if you would like to skip that extra string allocation, an overload
 *        is provided especially for you with a second 'optional' argument.
 *  @param data The pointer to the start of the binary data.
 *  @return Returns a base 64 encoded string.
 */
std::string encode(const ByteBuffer & data);

/** @copydoc Base64_encode
 *  @param cache_str If specified this function will reuse the pre-allocated,
 *                   internal buffer of this 'cache' string.
 */
std::string encode(const ByteBuffer & data, std::string & cache_str);

/** @class Base64_decode
 *  Decodes a base 64 string into a binary buffer and sets the provided
 *  varibles with this information. @n
 *  Excepts either "+/" or "-_"
 *  note: If you would like to skip that extra data allocation, an overload is
 *        provided especially for you with a second 'optional' argument.
 *  @param str The base 64 string to decode.
 *  @return Returns decoded data.
 */
ByteBuffer decode(const std::string & str);

/** @copydoc Base64_decode
 *  @param cache_data If specified this function will reuse the pre-allocated,
 *                    internal buffer of this 'cache' data.
 */
ByteBuffer decode(const std::string & str, ByteBuffer & cache_data);

} // end of Base64 namespace
