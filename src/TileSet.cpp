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

using Error        = std::runtime_error;
using InvArg       = std::invalid_argument;
using TiXmlElement = tinyxml2::XMLElement;
using XmlRange     = tmap::XmlRange;
using PropertyMap  = tmap::TileSet::PropertyMap;

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

std::vector<PropertyMap> load_tile_properties(const TiXmlElement *);

std::vector<std::string> load_tile_types(const TiXmlElement *);

} // end of <anonymous> namespace

namespace tmap {

/* static */ constexpr const int TileSetInterface::k_no_tile;

/* vtable anchcor */ TileSetInterface::~TileSetInterface() {}

// ----------------------------------------------------------------------------

TileSet::TileSet()
    { check_invarients(); }

TileSet::~TileSet() {}

void TileSet::set_referer(const std::string & referer)
    { m_referer = referer; }

bool TileSet::load_texture() {
    fix_file_path();

    m_texture = std::make_unique<sf::Texture>();
    if (!m_texture->loadFromFile(m_filename))
        return false;

    check_invarients();
    return true;
}

sf::IntRect TileSet::compute_texture_rect(TileFrame frame) const
    { return compute_texture_rect(frame.m_gid); }

sf::IntRect TileSet::compute_texture_rect(int gid) const {
    verify_owns_gid(gid, "compute_texture_rect");
    return texture_rectangle(gid - begin_gid());
}

TileEffect * TileSet::tile_effect_for(int gid) const
    { return m_tile_effects[std::size_t(convert_to_local_id(gid))]; }

const sf::Texture & TileSet::texture() const {
    if (m_texture) return *m_texture;
    throw Error("TileSet::texture: TileSet has no texture loaded.");
}

int TileSet::begin_gid() const { return m_begin_gid; }

int TileSet::end_gid() const { return m_end_gid; }

const PropertyMap * TileSet::properties_on_gid(int id) const {
    verify_owns_gid(id, "properties_on_gid");
    return properties_on(convert_to_local_id(id));
}

const PropertyMap * TileSet::properties_on(int tid) const {
    if (tid < 0) return nullptr;
    if (tid >= int(m_properties.size())) return nullptr;
    return &m_properties[std::size_t(tid)];
}

const std::string & TileSet::type_of(int tid) const {
    verify_owns_local_id(tid, "type_of");
    static const std::string k_empty;
    if (tid >= int(m_tile_types.size())) return k_empty;
    return m_tile_types[std::size_t(tid)];
}

void TileSet::load_from_xml(const TiXmlElement * el) {
    // may come from map file, a seperate tileset file, will not have this
    int first_gid = read_int_attribute(el, "firstgid");

    // check if el is just a TSX reference and fix that!
    TiXmlDocument tsx_doc; // may need to live as long as el
    el = follow_tsx(tsx_doc, el);

    // tileset MUST have all the following to be valid
    int spacing = 0;
    const char * source = nullptr;
    sf::Vector2i tile_size;
    std::unique_ptr<sf::Texture> texture_ = nullptr;
    std::vector<PropertyMap> properties;
    std::vector<std::string> tile_types;
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

        // this should be ignored I guess?
        // of course this seems odd/inconsistent behavior on TilEd's behavior
        // Why/How can I add a tile object whose id falls outside of the
        // tileset, with size being determined solely by XML data?
        source = img_src_el->Attribute("source");
        if (!source) {
            throw Error(make_error_header(el) + "No source image specified "
                        "for tileset.");
        }
        texture_ = std::make_unique<sf::Texture>();
        std::string fn = source;
        fix_path(fn, m_referer, fn);
        if (!texture_->loadFromFile(fn)) {
            throw Error("TileSet::load_from_xml: cannot load image \"" + fn + "\" (with referer \"" + m_referer + "\")");
        }
        properties = load_tile_properties(el);
        tile_types = load_tile_types(el);
    } catch (InvArg &) {
        throw Error(make_error_header(el) + "TileSet information contains "
                    "non-integers where integers were expected");
    }
    std::vector<TileEffect *> tile_effects;
    sf::Vector2i tileset_size = ::size_in_tiles(tile_size, sf::Vector2i(texture_->getSize()), spacing);
    tile_effects.resize(std::size_t(tileset_size.x*tileset_size.y),
                        &NoTileEffect::instance()                 );

    m_filename = source; // may throw
    fix_file_path();

    // these will not throw
    m_tile_effects.swap(tile_effects);
    m_texture     .swap(texture_);
    m_properties  .swap(properties);
    m_tile_types  .swap(tile_types);

    m_tile_size  = tile_size;
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
        const PropertyMap & prop_map = m_properties[i];
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

/* private */ void TileSet::fix_file_path() {
    if (m_referer.empty()) return;

    std::string fixed_filename;
    fix_path(m_filename, m_referer, fixed_filename);

    m_filename = fixed_filename;
    m_referer.clear();
}

/* private */ sf::Vector2i TileSet::size_in_tiles() const {
    return ::size_in_tiles(m_tile_size, sf::Vector2i(texture().getSize()), m_spacing);
}

/* private */ const TiXmlElement * TileSet::follow_tsx
    (TiXmlDocument & tsx_doc, const TiXmlElement * el) const
{
    const char * source_file = nullptr;
    if (!(source_file = el->Attribute("source"))) return el;

    std::string fn = source_file;
    fix_path(fn, m_referer, fn);
    load_xml_file(tsx_doc, fn.c_str());
    tinyxml2::XMLNode * first_child = tsx_doc.FirstChildElement("tileset");
    if (!first_child)
        throw Error(make_error_header(el) + "TSX file, has no child nodes.");
    if (!first_child->ToElement())
        throw Error(make_error_header(el) + "TSX file, cannot find tileset.");

    // loaded document ok
    return first_child->ToElement();
}

/* private */ int TileSet::convert_to_gid(int tid) const {
    if (tid < 0) return k_no_tile;
    if (tid > (end_gid() - begin_gid())) return k_no_tile;
    return tid + begin_gid();
}

/* private */ sf::IntRect TileSet::texture_rectangle(int local_id) const {
    // convert to tid
    verify_owns_local_id(local_id, "texture_rectangle");
    if (size_in_tiles().x == 0) {
        throw Error("Tileset \"" + m_filename + "\" "
                    "size is invalid (width is 0).");
    }

    int tile_x = local_id % size_in_tiles().x;
    int tile_y = local_id / size_in_tiles().x;

    sf::IntRect txt_rect;
    txt_rect.left   = tile_x*(m_tile_size.x + m_spacing);
    txt_rect.top    = tile_y*(m_tile_size.y + m_spacing);
    txt_rect.width  = m_tile_size.x;
    txt_rect.height = m_tile_size.y;

    return txt_rect;
}

/* private */ TileEffect * TileSet::get_effect(int tid) const {
    verify_owns_local_id(tid, "get_effect");
    return m_tile_effects[std::size_t(tid)];
}

/* private */ int TileSet::convert_to_local_id(int gid) const {
    verify_owns_gid(gid, "convert_to_local_id");
    return gid - begin_gid();
}

/* private */ void TileSet::verify_owns_gid(int gid, const char * caller) const {
    if (gid >= begin_gid() && gid < end_gid()) return;
    throw InvArg("TileSet::" + std::string(caller) + ": gid "
                 + std::to_string(gid) + " does not belong to this tileset "
                 "containing gids [" + std::to_string(begin_gid()) + " "
                 + std::to_string(end_gid()) + "].");
}

/* private */ void TileSet::verify_owns_local_id(int tid, const char * caller) const {
    if (tid < 0)
        throw InvArg("TileSet::" + std::string(caller) + " local id must be a positive integer.");
    if (tid < (end_gid() - begin_gid())) return;
    throw InvArg("TileSet::" + std::string(caller) + " given local id ("
                 + std::to_string(tid) + ") exceeds the maximum id value ("
                 + std::to_string(end_gid() - begin_gid()) + ")");
}

/* private */ void TileSet::check_invarients() const {
    assert(m_end_gid - m_begin_gid == int(m_tile_effects.size()));
}

} // end of tmap namespace

namespace {

template <typename T, typename Func>
std::vector<T> load_tiles(const TiXmlElement *, Func &&);

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

std::vector<PropertyMap> load_tile_properties(const TiXmlElement * tileset_el) {
    using XmlEl = TiXmlElement;
    return load_tiles<PropertyMap>(tileset_el, []
        (const XmlEl & tile_el, PropertyMap & props)
    {
        for (const XmlEl & props_el : XmlRange(tile_el, "properties")) {
            load_properties(props, &props_el);
        }
    });
}

std::vector<std::string> load_tile_types(const TiXmlElement * el) {
    using XmlEl = TiXmlElement;
    return load_tiles<std::string>(el, []
        (const XmlEl & tile_el, std::string & typename_)
    {
        const auto * gv = tile_el.Attribute("type");
        if (!gv) return;
        typename_ = gv;
    });
}

// ----------------------------------------------------------------------------

template <typename T, typename Func>
std::vector<T> load_tiles(const TiXmlElement * el, Func && f) {
    using XmlEl = TiXmlElement;
    std::vector<T> rv;
    for (const XmlEl & tile_el : XmlRange(el, "tile")) {
        std::size_t index = std::size_t(tmap::read_int_attribute(&tile_el, "id"));
        if (index >= rv.size()) {
            rv.resize(index + 1);
        }
        T & obj = rv[index];
        f(tile_el, obj);
    }
    return rv;
}

} // end of <anonymous> namespace
