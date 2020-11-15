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

#include <tmap/ZLib.hpp>

#include <vector>
#include <stdexcept>
#include <fstream>

#include <common/TypeList.hpp>

#include <cstddef>
#include <cassert>

#define ZLIB_CONST
#define ZLIB_WINAPI
#include <zlib.h> // heavily pollutes global namespace

using Error = std::runtime_error;

namespace {

using namespace ZLib;

void save_to_vector(UInt8 * buf, const std::size_t BUF_SIZE, ByteBuffer & data)
    { data.insert(data.end(), buf, buf + BUF_SIZE); }

const std::size_t TEMP_BUF_SIZE = 16384;

const char * const FAILED_TO_INIT_ZLIB_STATE_MSG =
    "Failed to initialize ZLib state.";

const char * const SHOULD_BE_UNREACHABLE_MSG =
    "flow control has reached an 'unreachable' segment of code, this is the "
    "programmer's fault, tell them about this error message.";

} // end of <anonymous> namespace

//
// ---------------- ZLib INFLATE/DECOMPRESSION IMPLEMENTATION ------------------
//

namespace {

void zlib_inflate_init(z_stream & strm);

class ZInflateRaii {
public:
    explicit ZInflateRaii(z_stream & strm): m_strm(&strm) {}
    ~ZInflateRaii() { (void)inflateEnd(m_strm); }
private:
    z_stream * m_strm;
};

} // end of <anonymous> namespace

namespace ZLib {

ByteBuffer decompress(const ByteBuffer & src_data, ByteBuffer & cache_data) {
    ByteBuffer out_data;
    out_data.swap(cache_data);
    out_data.clear();

    UInt8 output_buffer[TEMP_BUF_SIZE];

    // inflation state information
    z_stream strm;
    ZInflateRaii strm_ender(strm); (void)strm_ender; // keep g++ happy

    strm.avail_out = TEMP_BUF_SIZE;
    strm.next_out  = output_buffer;

    zlib_inflate_init(strm);

    strm.avail_in = static_cast<decltype(strm.avail_in)>(src_data.size());
    strm.next_in = static_cast<const Bytef *>(src_data.data());

    while (true) {
        // inflate until out of buffer space
        do {
            switch (inflate(&strm, Z_NO_FLUSH)) {
            case Z_OK: break;
            case Z_STREAM_END:
                save_to_vector(output_buffer, strm.total_out, out_data);
                return out_data;
            case Z_STREAM_ERROR:
                // state not clobbered, "stack corruption"
                throw Error("ZLib Decompression State is clobbered infrom the "
                            "developer.");
            case Z_NEED_DICT:
                // consider this an error, this is not handled
                throw Error("ZLib needs a dictionary, you must use more "
                            "advanced functions in order to use this feature.");
            case Z_DATA_ERROR: case Z_MEM_ERROR: default:
                throw Error(std::string("ZLib::decompress error occured: \"") +
                            strm.msg + std::string("\"."));
            }
        }
        while (strm.avail_out != 0);

        // out of space, reallocate
        save_to_vector(output_buffer, strm.total_out, out_data);
        strm.avail_out = TEMP_BUF_SIZE;
        strm.next_out  = output_buffer;
        strm.total_out = 0;
    }
    // cannot return here, loop never breaks, return points inside
    assert(false);
    throw Error(std::string("ZLib::decompress ") + SHOULD_BE_UNREACHABLE_MSG);
}

ByteBuffer decompress(const ByteBuffer & src_data) {
    ByteBuffer blank_vec;
    return decompress(src_data, blank_vec);
}

} // end of ZLib namespace

namespace {

void zlib_inflate_init(z_stream & strm) {
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    int ret = inflateInit(&strm);
    if (ret != Z_OK)
        throw Error(FAILED_TO_INIT_ZLIB_STATE_MSG);
}

} // end of <anonymous> namespace

//
// ----------------- ZLib DEFLATE/COMPRESSION IMPLEMENTATION -------------------
//

namespace {

void zlib_deflate_init(z_stream & strm, ZLib::CompressionLevel level);

class ZDeflateRaii {
public:
    explicit ZDeflateRaii(z_stream & strm): m_strm(&strm) {}
    ~ZDeflateRaii() { (void)deflateEnd(m_strm); }
private:
    z_stream * m_strm;
};

} // end of <anonymous> namespace

namespace ZLib {

ByteBuffer compress
    (const ByteBuffer & src, CompressionLevel level, ByteBuffer & cache_out)
{
    // I super apollogize for the long function!
    ByteBuffer data_out;
    cache_out.clear();
    data_out.swap(cache_out);

    UInt8 output_buffer[TEMP_BUF_SIZE];

    // initializes ZLib stream state
    z_stream strm; // stream state
    ZDeflateRaii strm_ender(strm); (void)strm_ender; // keep g++ happy
    zlib_deflate_init(strm, level);

    // now consider that the output memory runs out (ratio > 1, ew...)
    // assume: will be done immediately after one go with decompression
    int flush     = Z_NO_FLUSH;
    strm.avail_in = static_cast<decltype(strm.avail_in)>(src.size());
    strm.next_in  = static_cast<const Bytef *>(src.data());

    strm.avail_out = TEMP_BUF_SIZE;
    strm.next_out  = output_buffer;

    while (true) {
        bool need_more_buf = false;
        switch (deflate(&strm, flush)) {
        case Z_OK: break;
        case Z_BUF_ERROR:
            // not enough output is the only apporpiate case for which this
            // condidtion should be met
            need_more_buf = true;
            if (strm.avail_out == TEMP_BUF_SIZE) {
                // I'm really hoping this won't happen... but it probably will
                throw Error("ZLib, temp output buffer too small!");
            }
            break;
        case Z_STREAM_ERROR: // show stopper
            throw Error(std::string("ZLib error: \"") + strm.msg +
                        std::string("\"."));
        case Z_STREAM_END:
            save_to_vector(output_buffer, strm.total_out, data_out);
            return data_out;
        default:
            throw Error(std::string("ZLib::compress ") +
                        SHOULD_BE_UNREACHABLE_MSG);
        }

        if (need_more_buf || strm.avail_out == 0) {
            // :TODO: is there a way to find out how much ZLib needs?
            save_to_vector(output_buffer, strm.total_out, data_out);
            // reset output buf
            strm.avail_out = TEMP_BUF_SIZE;
            strm.next_out  = output_buffer;
            strm.total_out = 0;
            need_more_buf = false;
        }

        // tell ZLib to finish if there is nothing left to process
        if (strm.avail_in == 0)
            flush = Z_FINISH;
    }

    assert(false);
    throw Error(std::string("ZLib::compress ") + SHOULD_BE_UNREACHABLE_MSG);
}

ByteBuffer compress(const ByteBuffer & src, CompressionLevel level) {
    ByteBuffer cache_arg;
    return compress(src, level, cache_arg);
}

} // end of ZLib namespace

namespace {

void zlib_deflate_init(z_stream & strm, ZLib::CompressionLevel level) {
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    int ret = deflateInit(&strm, static_cast<int>(level));
    if (ret != Z_OK)
        throw Error(FAILED_TO_INIT_ZLIB_STATE_MSG);
}

} // end of <anonymous> namespace

namespace ZLib {

ByteBuffer dump_file_to_buffer(const char * filename) {
    ByteBuffer buff;
    std::ifstream fin(filename, std::ifstream::ate | std::ifstream::binary);
    auto filesize = fin.tellg();
    if (filesize == static_cast<decltype(filesize)>(-1)) {
        throw std::runtime_error(std::string("Failed to open file: \"") +
                                 filename + "\".");
    }
    fin.seekg(0, fin.beg);
    buff.resize(std::size_t(filesize));
    fin.read(reinterpret_cast<char *>(&buff[0]), filesize);
    fin.close();
    return buff;
}

void dump_buffer_to_file(const char * filename, const ByteBuffer & buff) {
    std::ofstream fout;
    fout.open(filename);
    fout.write(reinterpret_cast<const char *>(buff.data()), buff.size());
    fout.close();
}

} // end of ZLib namespace
