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

#include <tmap/Base64.hpp>

#include <stdexcept>

#include <cassert>

using Error = std::runtime_error;

namespace Base64 {

namespace /* anonymous */ {

// maps numeric values to their proper Base64 characters
const char * const k_encode_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "abcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_chunk
    (const ByteBuffer & data, std::string & str,
     std::size_t chunk_index, std::size_t chunk_size);

} // end of <anonymous> namespace

std::string encode(const ByteBuffer & data) {
    std::string cache_str;
    return encode(data, cache_str);
}

std::string encode(const ByteBuffer & data, std::string & cache_str) {
    std::string str;
    str.reserve((data.size()*4)/3);

    // let's say you don't want that extra nasty allocation :)
    cache_str.swap(str);
    str.clear();

    // encoding table
    std::size_t chunk_count = data.size() / 3;
    std::size_t rem_bytes   = data.size() % 3;

    // encode complete chunks
    // doing it this way prevents both code duplication and also allows g++ to
    // make use of one of it's common optimizations, emiting code specific to
    // a constant value
    for (std::size_t i = 0; i != chunk_count*3; i += 3)
        encode_chunk(data, str, i, 3);

    if (rem_bytes == 2) {
        encode_chunk(data, str, chunk_count, 2);
        str += "=";
    } else if (rem_bytes == 1) {
        encode_chunk(data, str, chunk_count, 1);
        str += "==";
    }
    return str;
}

namespace /* anonymous */ {

// sentinel value for the table below
const std::size_t k_bad_char = std::size_t(-1);

/* a table that maps all possible byte values, in this case Base64, ASCII
 * characters to thier numeric values, if a byte value is not a valid Base64
 * character it maps the sentinel "bad character value".
 *
 * note: An ASCII reference table is necessary to understanding this table
 *       (should fix it a bit in a future version?)
 */
const std::size_t k_decode_table[] = {
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, 62, // maps "+" to 62
    k_bad_char, k_bad_char, k_bad_char, 63, // maps "/" to 63
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char,
    // maps all upper case values to thier respective values
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char,
    // maps all lower case
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51,
    // the rest of the values map to the sentinel "bad character value"
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char, k_bad_char, k_bad_char,
    k_bad_char, k_bad_char, k_bad_char // all 256!
};
} // end of <anonymous> namespace

ByteBuffer decode(const std::string & str) {
    ByteBuffer cache_data;
    return decode(str, cache_data);
}

ByteBuffer decode(const std::string & str, ByteBuffer & cache_data) {
    using Byte = ByteBuffer::value_type;
    // the string must be divisible by four
    if (str.length() % 4 != 0) {
        throw Error("String not divisible by four.\n\"" + str + "\"\n"
                    "of length: " + std::to_string(str.length()));
    }

    ByteBuffer out_data;
    // swaping can prevent reallocation
    out_data.swap(cache_data);
    out_data.clear();
    out_data.reserve((str.length() / 4)*3);

    // check padding characters (limit 2)
    std::size_t rem_bytes = 0;
    while (str[str.length() - 1 - rem_bytes] == '=') {
        rem_bytes++;
        if (rem_bytes == 3)
            throw Error("Too many padding characters.");
    }

    for (std::size_t i = 0; i != str.length() - rem_bytes; i++) {
        // valid base 64 character?
        std::size_t val = k_decode_table[std::size_t(str[i])];
        if (val == k_bad_char) {
            throw Error("Bad character found found at position: " +
                        std::to_string(i) + " which is \"" + str[i] +
                        "\" (code: " + std::to_string(int(str[i])) + ")");
        }
        // which step are we on, there are four base 64 numbers per chunk
        // first bits are the most significant last -> least
        switch (i % 4) {
        case 0:
            // first six bits of the first byte
            out_data.push_back(Byte(val));
            out_data.back() <<= 2;
            break;
        case 1:
            // last two bits of the first byte, first four of the second
            out_data.back() |= (val >> 4);
            // these first four bits make up the more significant digits
            out_data.push_back(Byte((val & 0x0F) << 4));
            break;
        case 2:
            // last four bits of the second byte
            out_data.back() |= val >> 2;
            // first two bits of the third byte
            out_data.push_back(Byte((val & 0x03) << 6));
            break;
        case 3:
            // last six bits of the third byte
            out_data.back() |= val;
            break;
        default:
            assert(false);
            break;
        }
    }
    return out_data;
} // end of decode function

namespace /* anonymous */ {

void encode_chunk
    (const ByteBuffer & data, std::string & str,
     std::size_t chunk_index, std::size_t chunk_size)
{
    // does the chunk have size? if not there is nothing to append
    if (chunk_size == 0) return;

    // first is most significant, last -> least
    unsigned v;
    // first six bits of the first, trim leftmost 00's
    v = unsigned((data[chunk_index + 0] & 0xFC) >> 2);
    str += k_encode_table[v];
    // last two bits of the first byte, first 4 of the next
    v = unsigned((data[chunk_index + 0] & 0x03) << 4);
    // if we reached the end of the chunk, save and quit
    if (chunk_size == 1) {
        str += k_encode_table[v];
        return;
    }

    // encode second byte
    v |= unsigned((data[chunk_index + 1] & 0xF0) >> 4);
    str += k_encode_table[v];
    // last four bits of the second, first two of the third
    v = unsigned((data[chunk_index + 1] & 0x0F) << 2);
    if (chunk_size == 2) {
        str += k_encode_table[v];
        return;
    }

    // third and final byte
    v |= unsigned((data[chunk_index + 2] & 0xC0) >> 6);
    str += k_encode_table[v];
    // last six bits of the third byte
    v = unsigned(data[chunk_index + 2] & 0x3F);
    str += k_encode_table[v];
} // end of encode_chunk function

} // end of <anonymous> namespace

} // end of Base64 namespace
