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

#include "TiXmlHelpers.hpp"

#include <tmap/ZLib.hpp>

#include <SFML/System/Vector2.hpp>

#include <string>
#include <stdexcept>

#include <tinyxml2.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <cassert>

#undef major

using Error = std::runtime_error;

namespace {

template <typename T>
sf::Vector2<T> major(const sf::Vector2<T> & v) {
    return (v.x > v.y) ? sf::Vector2<T>(v.x, 0) : sf::Vector2<T>(0, v.y);
}

}

namespace tmap {

void do_stuff() {
    using VectorD = sf::Vector2<double>;
    VectorD v = major(VectorD(90.0, 23.0));
    (void)v;
}

int read_int_attribute(const TiXmlElement * el, const char * name) {
    const char * attr_str = el->Attribute(name);
    if (!attr_str)
        throw Error(std::string("Attribute ") + name + " does not exist.");
    return std::stoi(attr_str);
}

} // end of tmap namespace

namespace {

bool cstr_ends_with(const char * haystack, const char * needle);

bool check_file_exist(const char * filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

bool check_file_exist(const std::string & filename) {
    return check_file_exist(filename.c_str());
}

} // end of <anonymous> namespace

namespace tmap {

void load_xml_file(TiXmlDocument & doc, const char * filename) {
    using namespace ZLib;
    tinyxml2::XMLError gv;
    // check if tmxz format, by extension
    if (cstr_ends_with(filename, ".tmxz")) {
        ByteBuffer buff = dump_file_to_buffer(filename);
        buff = ZLib::decompress(buff);
        // parse as ASCII
        gv = doc.Parse
            (reinterpret_cast<const char *>(buff.data()), buff.size());
    } else {
        if (check_file_exist(filename)) {
            gv = doc.LoadFile(filename);
        } else {
            std::string zfilename = filename;
            zfilename += "z";
            if (check_file_exist(zfilename)) {
                ByteBuffer buff = dump_file_to_buffer(zfilename.c_str());
                buff = ZLib::decompress(buff);
                // parse as ASCII
                gv = doc.Parse
                    (reinterpret_cast<const char *>(buff.data()), buff.size());
            } else {
                gv = tinyxml2::XML_ERROR_FILE_NOT_FOUND;
            }
        }
    }

    if (gv != tinyxml2::XML_SUCCESS)
        throw Error(std::string("Failed to open file: ") + filename);
}

TiXmlIter::TiXmlIter(): el(nullptr), name(nullptr) {}
TiXmlIter::TiXmlIter(const TiXmlElement * el_, const char * name_):
    el(el_), name(name_) {}
TiXmlIter & TiXmlIter::operator ++ ()
    { el = el->NextSiblingElement(name); return *this; }
bool TiXmlIter::operator != (const TiXmlIter & rhs) const
    { return el != rhs.el; }
const TiXmlElement & TiXmlIter::operator * () const
    { return *el; }

XmlRange::XmlRange(const TiXmlElement * el_, const char * name_):
    m_beg(el_->FirstChildElement(name_), name_) {}
XmlRange::XmlRange(const TiXmlElement & el_, const char * name_):
    m_beg(el_.FirstChildElement(name_), name_) {}
TiXmlIter XmlRange::begin() const { return m_beg;       }
TiXmlIter XmlRange::end()   const { return TiXmlIter(); }

} // end of tmap namespace

namespace {

bool cstr_ends_with(const char * haystack, const char * needle) {
    assert(::strlen(needle  ) > 0);
    assert(::strlen(haystack) > 0);
    const char * needle_last   = needle   + (::strlen(needle  ) - 1);
    const char * haystack_last = haystack + (::strlen(haystack) - 1);
    while (true) {
        if (*needle_last != *haystack_last)
            return false;
        // at this point: we know last chars are equal
        --needle_last;
        --haystack_last;

        // reached the begining of needle
        if (needle_last == needle)
            return true;
        // reached the begining of haystack
        if (haystack_last == haystack)
            return false;
    }
}

} // end of <anonymous> namespace
