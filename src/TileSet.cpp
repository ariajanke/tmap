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

#include "TileSet.hpp"
#include "TiXmlHelpers.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <tinyxml2.h>

#include <stdexcept>
#include <cassert>

namespace {

using Error = std::runtime_error;
using TiXmlElement = tinyxml2::XMLElement;
using XmlRange = tmap::XmlRange;

bool is_dir_slash(char c) { return c == '\\' || c == '/'; }

// dupelicate exists in LuaHelpers
// however, merging them is complicated, tmap is meant to be seperate
void fix_path
    (const std::string & referee, const std::string & referer,
     std::string & dest_path);

std::string make_error_header(const TiXmlElement * el);

void load_properties
    (std::map<std::string, std::string> & props, const TiXmlElement * props_el);

sf::Vector2i size_in_tiles
    (const sf::Vector2i & tile_size, const sf::Vector2i & image_size,
     int spacing);

} // end of <anonymous> namespace

namespace tmap {

TileSet::TileSet():
    m_tile_size(),
    m_image_size(),
    m_filename(),
    m_begin_gid(0),
    m_end_gid(0),
    m_spacing(0),
    m_texture()
{ check_invarients(); }

TileSet::~TileSet(){}

void TileSet::set_referer(const std::string & referer) {
    m_referer = referer;
}

bool TileSet::load_texture() {
    fix_file_path();

    if (!m_texture.loadFromFile(m_filename))
        return false;

    m_image_size = sf::Vector2i(int(m_texture.getSize().x),
                                int(m_texture.getSize().y));
    check_invarients();
    return true;
}

sf::IntRect TileSet::compute_texture_rect(TileFrame frame) const
    { return compute_texture_rect(frame.m_gid); }

sf::IntRect TileSet::compute_texture_rect(int gid) const {
    if (size_in_tiles().x == 0) {
        throw Error("Tileset \"" + m_filename + "\" "
                    "size is invalid (width is 0).");
    }
    int local_id = gid - begin_gid();
    int tile_x = local_id % size_in_tiles().x;
    int tile_y = local_id / size_in_tiles().x;

    sf::IntRect txt_rect;
    txt_rect.left   = tile_x*(m_tile_size.x + m_spacing);
    txt_rect.top    = tile_y*(m_tile_size.y + m_spacing);
    txt_rect.width  = m_tile_size.x;
    txt_rect.height = m_tile_size.y;

    return txt_rect;
}

TileEffect * TileSet::tile_effect_for(int gid) const {
    int local_id = gid - begin_gid();
    return m_tile_effects[std::size_t(local_id)];
}

int TileSet::begin_gid() const { return m_begin_gid; }

int TileSet::end_gid() const { return m_end_gid; }

const TileSet::ProperiesMap * TileSet::properties_on_gid(int id) const {
    id -= begin_gid();
    assert(id >= 0);
    if (id >= int(m_properties.size())) return nullptr;
    return &m_properties[std::size_t(id)];
}

void TileSet::load_from_xml(const TiXmlElement * el) {

    // may come from map file, a seperate tileset file, will not have this
    int first_gid;
    first_gid = read_int_attribute(el, "firstgid");

    // check if el is just a TSX reference and fix that!
    const char * source_file;
    TiXmlDocument tsx_doc; // may need to live as long as el
    if ((source_file = el->Attribute("source"))) {
        std::string fn = source_file;
        fix_path(fn, m_referer, fn);
        load_xml_file(tsx_doc, fn.c_str());
        tinyxml2::XMLNode * first_child = tsx_doc.FirstChildElement("tileset");
        if (!first_child)
            throw Error(make_error_header(el) + "TSX file, has no child nodes.");
        if (!first_child->ToElement())
            throw Error(make_error_header(el) + "TSX file, cannot find tileset.");

        // loaded document ok
        el = first_child->ToElement();
    }

    // tileset MUST have all the following to be valid
    int spacing = 0;
    const char * source;
    sf::Vector2i tile_size;
    sf::Vector2i image_size;

    try {
        tile_size.x = read_int_attribute(el, "tilewidth" );
        tile_size.y = read_int_attribute(el, "tileheight");

        // optional arguement
        // spacing (default is 0)
        if (el->Attribute("spacing"))
            spacing = std::stoi(el->Attribute("spacing"));

        // next we need an image source
        const TiXmlElement * img_src_el = el->FirstChildElement("image");

        if (!img_src_el)
            throw Error(make_error_header(el) + "TileSets requires an image.");

        // next we need image's attributes (all of them!)
        image_size.x = read_int_attribute(img_src_el, "width" );
        image_size.y = read_int_attribute(img_src_el, "height");
        source = img_src_el->Attribute("source");
        if (!source)
            throw Error(make_error_header(el) + "No source image specified "
                        "for tileset.");
        load_tile_properties(el, first_gid);
    } catch (std::invalid_argument &) {
        throw Error(make_error_header(el) + "TileSet information contains "
                    "non-integers where integers were expected");
    }
    std::vector<TileEffect *> tile_effects;
    sf::Vector2i tileset_size = ::size_in_tiles(tile_size, image_size, spacing);
    tile_effects.resize(std::size_t(tileset_size.x*tileset_size.y),
                        &NoTileEffect::instance()                 );

    m_filename = source; // may throw
    fix_file_path();

    // these will not throw
    m_tile_effects.swap(tile_effects);
    m_tile_size  = tile_size;
    m_image_size = image_size;
    m_spacing    = spacing;
    m_begin_gid  = first_gid;
    // must be done last (spacing and tile size)
    m_end_gid    = first_gid + tileset_size.x*tileset_size.y;
    assert(tileset_size == size_in_tiles());
    assert(m_tile_effects.size() >= m_properties.size());
    check_invarients();
}

void TileSet::set_tile_effect
    (const char * name, const char * value, TileEffect * te)
{
    assert(m_tile_effects.size() >= m_properties.size());
    const std::size_t end_index = m_properties.size();
    for (std::size_t i = 0; i != end_index; ++i) {
        const ProperiesMap & prop_map = m_properties[i];
        auto itr = prop_map.find(name);
        if (itr == prop_map.end()) continue;
        if (itr->second != value && value[0] != '\0') continue;
        m_tile_effects[i] = te;
    }
    check_invarients();
}

TileSet::IterValuePair TileSet::find_tile_effect_ref_and_name
    (const char * name, IterValuePair prev)
{
    assert(prev.tile_set_index >= 0);
    auto * first = &m_tile_effects.front();
    auto * end   = first + m_tile_effects.size();
    if (prev.tile_effect) {
        // tile effect must be in this tile set
        assert(prev.tile_effect >= first                 &&
               prev.tile_effect <  &m_tile_effects.back()  );
        first = prev.tile_effect + 1;
    }
    while (first != &m_tile_effects.back()) {
        assert(first - &m_tile_effects.front() >= 0);
        const std::size_t index = std::size_t(first - &m_tile_effects.front());
        if (index == m_properties.size())
            break;
        const auto & props = m_properties[index];
        auto pmap_itr = props.find(name);
        if (pmap_itr == props.end()) {
            ++first;
            continue;
        } else {
            IterValuePair rv = prev;
            int gid = int(end - first) + m_begin_gid;
            rv.tile_effect = first;
            rv.value = &pmap_itr->second;
            rv.tile_frame = TileFrame::construct_privately(gid);
            check_invarients();
            return rv;
        }
    }
    check_invarients();
    // reached the end?
    return IterValuePair();
}

void TileSet::load_tile_properties(const TiXmlElement * tileset_el, int) {
    using XmlEl = TiXmlElement;
    for (const XmlEl & tile_el : XmlRange(tileset_el, "tile")) {
        std::size_t index = std::size_t(read_int_attribute(&tile_el, "id"));
        if (index >= m_properties.size()) {
            m_properties.resize(index + 1);
        }
        ProperiesMap & props = m_properties[index];
        for (const XmlEl & props_el : XmlRange(tile_el, "properties")) {
            load_properties(props, &props_el);
        }
    }
    check_invarients();
}

/* private */ void TileSet::check_invarients() const {
    assert(m_end_gid - m_begin_gid == int(m_tile_effects.size()));
    //assert(m_tile_effects.size()   == m_properties.size()       );
}

/* private */ void TileSet::fix_file_path() {
    if (m_referer.empty()) return;

    std::string fixed_filename;
    fix_path(m_filename, m_referer, fixed_filename);

    m_filename = fixed_filename;
    m_referer.clear();
}

/* private */ sf::Vector2i TileSet::size_in_tiles() const {
    return ::size_in_tiles(m_tile_size, m_image_size, m_spacing);
}

} // end of rj namespace

namespace {

void fix_path
    (const std::string & referee, const std::string & referer,
     std::string & dest_path)
{
    // parameter checking
    bool is_path = false;
    for (char c : referer) {
        if (is_dir_slash(c)) {
            is_path = true;
            break;
        }
    }

    if (!is_path) {
        dest_path = referee;
        return;
    }

    // ex: referee "./path/data.txt", referer: "~/cat/dog.xml"
    // out: "~/cat/path/data.txt"
    auto itr = referer.end();
    while (!is_dir_slash(*--itr)) {}
    if (itr == referer.begin()) return;
    dest_path = referee;
    dest_path.insert(dest_path.begin(), referer.begin(), itr + 1);
}

std::string make_error_header(const TiXmlElement * el) {
    const char * name = el->Attribute("name");
    if (!name) name = "<anonymous>";
    return std::string("An error has occured while loaded the tileset \"") +
           name + std::string("\": ");
}

void load_properties
    (std::map<std::string, std::string> & props, const TiXmlElement * props_el)
{
    for (const TiXmlElement & el : XmlRange(props_el, "property")) {
        if (!el.Attribute("name") || !el.Attribute("value"))
            throw std::runtime_error("Both name and value must be specified.");
        props[el.Attribute("name")] = el.Attribute("value");
    }
}

sf::Vector2i size_in_tiles
    (const sf::Vector2i & tile_size, const sf::Vector2i & image_size,
     int spacing)
{
    if (tile_size.x == 0 || tile_size.y == 0) return sf::Vector2i();
    return sf::Vector2i(image_size.x / (tile_size.x + spacing),
                        image_size.y / (tile_size.y + spacing));
}

} // end of <anonymous> namespace
