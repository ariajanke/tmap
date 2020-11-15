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

namespace tinyxml2 {

class XMLDocument;
class XMLElement;

}

namespace tmap {

using TiXmlElement  = tinyxml2::XMLElement ;
using TiXmlDocument = tinyxml2::XMLDocument;

int read_int_attribute(const TiXmlElement * el, const char * name);

// allows use of 'zedified' file
void load_xml_file(TiXmlDocument & doc, const char * filename);

class TiXmlIter {
public:
    TiXmlIter();
    TiXmlIter(const TiXmlElement * el_, const char * name_);
    TiXmlIter & operator ++ ();
    bool operator != (const TiXmlIter & rhs) const;
    const TiXmlElement & operator * () const;
private:
    const TiXmlElement * el;
    const char * name;
};

class XmlRange {
public:
    XmlRange(const TiXmlElement * el_, const char * name_);
    XmlRange(const TiXmlElement & el_, const char * name_);
    TiXmlIter begin() const;
    TiXmlIter end()   const;
private:
    TiXmlIter m_beg;
};

} // end of namespace tmap
