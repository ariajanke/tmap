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

#include "TiledMapImpl.hpp"
#include "ColorLayer.hpp"
#include "TileSet.hpp"
#include "TileLayer.hpp"
#include "TiXmlHelpers.hpp"

#include <common/StringUtil.hpp>

#include <SFML/Graphics.hpp>

#include <tinyxml2.h>

#include <stdexcept>
#include <memory>

#include <cassert>

namespace {

using TiXmlDocument     = tinyxml2::XMLDocument;
using TiXmlElement      = tinyxml2::XMLElement;
using Error             = std::runtime_error;
using MapLayer          = tmap::MapLayer;
using TileSetPtr        = tmap::TiledMapImpl::TileSetPtr;
using ConstTileSetPtr   = tmap::TiledMapImpl::ConstTileSetPtr;
using TileSetPtrVector  = tmap::TiledMapImpl::TileSetPtrVector;
using PropertyMap       = tmap::TiledMapImpl::PropertyMap;
using MapObject         = tmap::MapObject;
using MapLayerIter      = tmap::TiledMapImpl::MapLayerIter;
using MapLayerConstIter = tmap::TiledMapImpl::MapLayerConstIter;
using XmlRange          = tmap::XmlRange;

sf::Color read_color_from(const TiXmlElement * el, const char * attr_name);

void load_whole_map_properties(const TiXmlElement * el, PropertyMap & map);

/** Loads a map object (from Tiled Object layers) from the given element
 *  (whose tag is "object").
 *  All attributes (including name and type) outside of the object bounds
 *  (x, y, width and height) are optional. In the case that name or type is
 *  missing, they will be left blank.
 *  @throw Throws if width or height are negative.
 *  @warning Assertion: the provided element is an "object" tag.
 *  @note (This may become universal) @n
 *        bad_alloc throws do not make the strong gruantee, with the rationale
 *        that if memory allocation fails, then there is little point in
 *        keeping the program running.
 *  @param el  XML element to load the object data from.
 *  @param tilesets Some map objects rely on tilesets, so called "tile objects".
 *  @param obj The object to load the XML into, any previous state will be
 *             erased.
 */
void load_map_object_properties
    (const tinyxml2::XMLElement * el, const TileSetPtrVector & tilesets,
     MapObject & obj);

TileSetPtr find_tile_set_for_gid(const TileSetPtrVector &, int gid) noexcept;

} // end of <anonymous> namespace

namespace tmap {

TiledMapImpl::TiledMapImpl():
    m_map_width   (0      ),
    m_map_height  (0      ),
    m_tile_width  (0      ),
    m_tile_height (0      ),
    m_ground_layer(nullptr)
{}

TiledMapImpl::~TiledMapImpl() {}

void TiledMapImpl::load_from_file(const char * filename) {
    TiXmlDocument doc;
    load_xml_file(doc, filename);

    const TiXmlElement * map_el = doc.FirstChildElement("map");
    if (!map_el)
        throw Error("No map root element.");

    // integer reading requirments, interface must provide some sort of error
    // information if conversion fails
    // - the XML library returns 0 if integer conversion fails or the int,
    //   this is an issue because 0 is a perfectly valid and expected value
    //   for my purposes
    // interface needs to work with c-string types provided by the XML library

    // map globals... tile width, tile height, width, height
    int map_width, map_height, tile_width, tile_height;

    try {
        map_width   = read_int_attribute(map_el, "width");
        map_height  = read_int_attribute(map_el, "height");
        tile_width  = read_int_attribute(map_el, "tilewidth");
        tile_height = read_int_attribute(map_el, "tileheight");
    } catch (std::invalid_argument &) {
        throw Error("Global map attribute(s) are non-integer(s).");
    }

    const char * orientation = nullptr;
    orientation = map_el->Attribute("orientation");
    if (!orientation)
        throw Error("Orientation is required for Tiled maps.");
    if (::strcmp(orientation, "orthogonal") != 0)
        throw Error("tmap only supports orthogonal maps.");

    load_whole_map_properties
        (map_el->FirstChildElement("properties"), m_whole_map_properties);

    // load layers
    std::vector<std::unique_ptr<MapLayer>> loaded_layers;

    // first layer -> color layer
    // TilEd maps always has a Color layer, for RJ they have a default color
    // whether or not TilEd specifies one

    {
    sf::Color out;
    if (!map_el->Attribute("backgroundcolor"))
        out = sf::Color::Black;
    else
        out = read_color_from(map_el, "backgroundcolor");

    auto cl = std::make_unique<ColorLayer>();
    cl->set_color(out);
    loaded_layers.emplace_back(std::move(cl));
    }

    // *** design issues:
    // ownership of tilesets
    // this function is too damn long
    // we need tilesets to render tiles, namely tile layers

    // tilesets
    TileSetPtrVector tileset_ptrs;
    for (const TiXmlElement & tileset_el : XmlRange(map_el, "tileset")) {
        tileset_ptrs.push_back(std::make_shared<TileSet>());
        TileSetPtr ts = tileset_ptrs.back();
        ts->set_referer(filename);
        ts->load_from_xml(&tileset_el);
        ts->load_texture();
    }
    std::sort(tileset_ptrs.begin(), tileset_ptrs.end(),
              [](const TileSetPtr & lhs, const TileSetPtr & rhs)
    { return lhs->begin_gid() < rhs->begin_gid(); });

    // tile layers
    TilePropertiesInterface * loaded_ground_layer = nullptr;
    for (const auto * layer_itr_el = map_el->FirstChildElement("layer");
         layer_itr_el; layer_itr_el = layer_itr_el->NextSiblingElement("layer"))
    {
        auto tl = std::make_unique<TileLayer>();

        tl->load_from_xml(layer_itr_el, tileset_ptrs);
        // tile layers don't know the tile size (which is global), so it must
        // be set seperately
        tl->set_tile_size(float(tile_width), float(tile_height));

        loaded_layers.emplace_back(std::move(tl));
    }

    load_map_objects(map_el, tileset_ptrs);

    m_layers.reserve(loaded_layers.size());

    m_name_to_draw_layer.reserve(loaded_layers.size());
    m_name_to_tile_layer.reserve(loaded_layers.size());
    m_drawable_layers.reserve(loaded_layers.size());

    // ----------------- no exceptions beyond this point ----------------------

    // transfer loaded content into map state (no more exceptions possible)
    // If I was proper careful, I'd use temporaries again... no time atm
    for (std::unique_ptr<MapLayer> & uptr : loaded_layers) {
        m_layers.emplace_back(uptr.release());

        const auto & name = m_layers.back()->name();
        if (name == "") continue;

        auto * tile_layer = dynamic_cast<TileLayer *>(&*m_layers.back());
        if (!tile_layer) continue;

        if (m_name_to_tile_layer.find(name) != m_name_to_tile_layer.end())
        { continue; }

        m_name_to_tile_layer[name] = tile_layer;
    }

    for (const auto & layerptr : m_layers) {
        m_drawable_layers.push_back(layerptr.get());
    }

    for (auto itr = m_layers.begin(); itr != m_layers.end(); ++itr) {
        const auto & name = (**itr).name();
        if (name == "") continue;
        auto ditr = m_drawable_layers.begin() + std::distance(m_layers.begin(), itr);
        m_name_to_draw_layer.insert(std::make_pair(name, ditr));
    }

    m_ground_layer = loaded_ground_layer;

    m_map_width   = map_width  ;
    m_map_height  = map_height ;
    m_tile_width  = tile_width ;
    m_tile_height = tile_height;
    m_tile_sets.swap(tileset_ptrs);
}

void TiledMapImpl::set_translation(const sf::Vector2f & offset) {
    for (std::unique_ptr<MapLayer> & map_layer : m_layers) {
        map_layer->set_translation(offset.x, offset.y);
    }
}

void TiledMapImpl::assign_tile_effect_with_property_pair
    (const char * name, const char * value, TileEffect * te)
{
    if (!name || !value || !te) {
        throw Error("TiledMapImpl::assign_tile_effect_with_property_pair: "
                    "while it is possible to pass a nullptr for any argument "
                    "it is still not meaningful to this function.");
    }

    for (std::shared_ptr<TileSet> & tset_ptr : m_tile_sets) {
        tset_ptr->set_tile_effect(name, value, te);
    }
}

const TilePropertiesInterface * TiledMapImpl::find_tile_layer
    (const std::string & name) const
{
    auto itr = m_name_to_tile_layer.find(name);
    if (itr == m_name_to_tile_layer.end()) return nullptr;
    return itr->second;
}

TilePropertiesInterface * TiledMapImpl::find_tile_layer(const std::string & name) {
    const auto & cthis = *this;
    return const_cast<TilePropertiesInterface *>(cthis.find_tile_layer(name));
}

const TiledMapImpl::PropertyMap & TiledMapImpl::map_properties() const {
    return m_whole_map_properties;
}

const TiledMapImpl::MapObjectContainer & TiledMapImpl::map_objects() const {
    return m_map_objects;
}

MapLayerIter TiledMapImpl::begin() { return m_drawable_layers.begin(); }

MapLayerIter TiledMapImpl::end() { return m_drawable_layers.end(); }

MapLayerConstIter TiledMapImpl::begin() const
    { return m_drawable_layers.begin(); }

MapLayerConstIter TiledMapImpl::end() const
    { return m_drawable_layers.end(); }

MapLayerIter TiledMapImpl::find_layer
    (const std::string & name, MapLayerIter pos)
{
    // pos is not a hint
    // example
    // I draw all layers up to "treetops" then all layers up to "ground" usw
    if (pos == MapLayerIter()) pos = begin();
    return find_lowest_above<false>(m_name_to_draw_layer.equal_range(name), pos, end());
}

MapLayerIter TiledMapImpl::find_layer
    (const char * name, MapLayerIter pos)
{
    if (pos == MapLayerIter()) pos = begin();
    return find_lowest_above<false>(m_name_to_draw_layer.equal_range(name), pos, end());
}

MapLayerConstIter TiledMapImpl::find_layer
    (const std::string & name, MapLayerConstIter pos) const
{
    if (pos == MapLayerConstIter()) pos = begin();
    return find_lowest_above<true>(m_name_to_draw_layer.equal_range(name), pos, end());
}

MapLayerConstIter TiledMapImpl::find_layer
    (const char * name, MapLayerConstIter pos) const
{
    if (pos == MapLayerConstIter()) pos = begin();
    return find_lowest_above<true>(m_name_to_draw_layer.equal_range(name), pos, end());
}

TiledMapImpl::IterValuePair TiledMapImpl::find_tile_effect_ref_and_name
    (const char * name, const IterValuePair & prev)
{
    IterValuePair rv = prev;
    if (&prev == &TileEffectAssignmentPriv::k_start_iter) {
        rv.tile_set_index = 0;
    }
    while (true) {
        auto & tile_set = *m_tile_sets[std::size_t(rv.tile_set_index)];
        rv = tile_set.find_tile_effect_ref_and_name(name, rv);
        if (rv.tile_effect)
            return rv;
        if (rv.tile_set_index == -1)
            break;
        ++rv.tile_set_index;
    }
    return IterValuePair();
}

ConstTileSetPtr TiledMapImpl::get_tile_set_for_gid(int gid) const noexcept
    { return find_tile_set_for_gid(m_tile_sets, gid); }

/* private */ void TiledMapImpl::load_map_objects
    (const TiXmlElement * map_el, const TileSetPtrVector & tilesets)
{
    for (const TiXmlElement & obj_group : XmlRange(map_el, "objectgroup")) {
        for (const TiXmlElement & obj : XmlRange(obj_group, "object")) {
            MapObject mobj;
            load_map_object_properties(&obj, tilesets, mobj);
            m_map_objects.push_back(MapObject());
            mobj.swap(m_map_objects.back());
        }
    }
}

} // end of tmap namespace

namespace {

void load_map_object_common_properties(const tinyxml2::XMLElement * el, MapObject & obj);

bool check_and_load_map_object_gid
    (const tinyxml2::XMLElement * el, const TileSetPtrVector & tilesets,
     MapObject & obj);

void load_map_object_shape(const tinyxml2::XMLElement * el, MapObject & obj);

sf::Color read_color_from(const TiXmlElement * el, const char * attr_name) {
    using UInt8 = sf::Uint8;
    sf::Color out;
    try {
        const char * color_str = el->Attribute(attr_name);
        if (!color_str)
            throw Error(std::string("Attribute with name \"") + attr_name +
                        "\" does not exist.");
        int len = int(::strlen(color_str));
        if (len != 4 && len != 7)
            throw Error("Color string is of invalid length.");
        if (*color_str != '#')
            throw Error("Color string must begin with '#' character.");
        ++color_str;
        int color_code = std::stoi(color_str, nullptr, 16);
        int mask = (len == 4) ? 0xF : 0xFF;
        int shift_amount = (len == 4) ? 4 : 8;
        out.b = UInt8(mask & color_code);
        out.g = UInt8(mask & (color_code >> shift_amount));
        out.r = UInt8(mask & (color_code >> shift_amount*2));
    } catch (std::invalid_argument &) {
        throw Error("Improperly formated RGB color code. Acceptable formats "
                    "are: #XXX or #XXXXXX, where X represents a hexidecimal "
                    "digit.");
    }
    return out;
}

void load_whole_map_properties(const TiXmlElement * el, PropertyMap & map) {
    if (!el) return;
    for (const TiXmlElement & prop : XmlRange(el, "property")) {
        const char * name  = prop.Attribute("name");
        const char * value = prop.Attribute("value");
        if (!name || !value) continue;
        map[name] = value;
    }
}

void load_map_object_properties
    (const tinyxml2::XMLElement * el, const TileSetPtrVector & tilesets,
     MapObject & obj)
{
    assert(::strcmp("object", el->Value()) == 0);

    load_map_object_common_properties(el, obj);
    bool has_gid = check_and_load_map_object_gid(el, tilesets, obj);
    load_map_object_shape(el, obj);
    if (obj.shape_type != MapObject::k_rectangle && has_gid) {
        throw Error("load_map_object_properties: map object cannot be "
                    "non-rectangular and have a gid associated with it.");
    }
}

TileSetPtr find_tile_set_for_gid(const TileSetPtrVector & tilesets, int gid) noexcept {
    auto itr = std::lower_bound(tilesets.begin(), tilesets.end(), gid,
        [](const TileSetPtr & lhs, int gid)
        { return lhs->end_gid() <= gid; });

    if (itr == tilesets.end()) return nullptr;
    return *itr;
}

// ----------------------------------------------------------------------------

void set_optional_builtin_attr(std::string & str, const char * cstr);

MapObject::PointVector read_points(const tinyxml2::XMLElement *);

void load_map_object_common_properties
    (const tinyxml2::XMLElement * el, MapObject & obj)
{
    sf::Rect<float> bounds;

    el->QueryFloatAttribute("x"     , &bounds.left  );
    el->QueryFloatAttribute("y"     , &bounds.top   );
    el->QueryFloatAttribute("width" , &bounds.width );
    el->QueryFloatAttribute("height", &bounds.height);

    if (bounds.width < 0.f || bounds.height < 0.f) {
        throw Error("The width and height of a map object may not be negative");
    }

    obj.bounds = bounds;
    set_optional_builtin_attr(obj.name, el->Attribute("name"));
    set_optional_builtin_attr(obj.type, el->Attribute("type"));

    obj.custom_properties.clear();
    for (const TiXmlElement & pel : XmlRange(el, "properties")) {
        for (const TiXmlElement & ppel : XmlRange(pel, "property")) {
            const char * name  = ppel.Attribute("name" );
            const char * value = ppel.Attribute("value");
            if (!name || !value) continue;
            obj.custom_properties.insert({ name, value });
        }
    }
}

bool check_and_load_map_object_gid
    (const tinyxml2::XMLElement * el, const TileSetPtrVector & tilesets,
     MapObject & obj)
{
    int gid = 0;
    if (el->QueryIntAttribute("gid", &gid) != tinyxml2::XML_SUCCESS)
        { return false; }

    // empty tile object
    if (gid == 0) { return true; }

    static const char * const k_gid_not_found =
        "check_and_load_map_object_gid: gid is not in range of any tileset.";

    const auto & tileset_ptr = find_tile_set_for_gid(tilesets, gid);
    if (!tileset_ptr) {
        throw Error(k_gid_not_found);
    }
    const auto & tileset = *tileset_ptr;
    if (tileset.begin_gid() > gid || tileset.end_gid() <= gid) {
        throw Error(k_gid_not_found);
    }
    assert(tileset.begin_gid() <= gid && tileset.end_gid() > gid);
    obj.local_tile_id = gid - tileset.begin_gid();
    obj.tile_set = tileset_ptr;
    // TilEd is weird here, the y position starts at the bottom of the object
    // I want to make it one-to-one with how it appears in the editor
    obj.bounds.top -= obj.bounds.height;

    if (obj.type.empty())
        obj.type = tileset.type_of(obj.local_tile_id);
    return true;
}

void load_map_object_shape(const tinyxml2::XMLElement * el, MapObject & obj) {
    auto * polygon_el  = el->FirstChildElement("polygon" );
    auto * ellipse_el  = el->FirstChildElement("ellipse" );
    auto * polyline_el = el->FirstChildElement("polyline");
    auto * text_el     = el->FirstChildElement("text"    );
    // there may only be one
    bool any_set = false;
    for (auto ptr : { polygon_el, ellipse_el, polyline_el, text_el }) {
        if (ptr) {
            if (any_set) {
                throw Error("MapObjects may only contain one of the following "
                            "elements: \"polygon\", \"ellipse\", \"polyline\", "
                            "or \"text\".");
            }
            any_set = true;
        }
    }
    if (polygon_el) {
        obj.shape_type = MapObject::k_polygon;
        obj.points = read_points(polygon_el);
    } else if (ellipse_el) {
        obj.shape_type = MapObject::k_ellipse;
    } else if (polyline_el) {
        obj.shape_type = MapObject::k_polyline;
        obj.points = read_points(polyline_el);
    } else if (text_el) {
        obj.shape_type = MapObject::k_text;
    } else if (obj.bounds.width != 0.f && obj.bounds.height != 0.f) {
        obj.shape_type = MapObject::k_rectangle;
    }
}

// ----------------------------------------------------------------------------

inline bool is_space(char c) { return c == ' '; }
inline bool is_comma(char c) { return c == ','; }

void set_optional_builtin_attr(std::string & str, const char * cstr)
    { str = (cstr) ? cstr : ""; }

MapObject::PointVector read_points(const tinyxml2::XMLElement * el) {
    MapObject::PointVector points;

    const char * point_string = el->Attribute("points");
    if (!point_string) return points;

    auto pt_str_end = point_string + ::strlen(point_string);
    points.reserve(std::size_t((pt_str_end - point_string) / 3));

    for_split<is_space>(point_string, pt_str_end,
                        [&points](const char * beg, const char * end)
    {
        static constexpr const char * const k_exactly_two_msg =
            "MapObject tuples must have exactly two numbers.";
        MapObject::PointVector::value_type v;
        auto list = { &v.x, &v.y };
        auto itr = list.begin();
        auto end_list = list.end();
        for_split<is_comma>(beg, end,
                            [&itr, end_list](const char * beg, const char * end)
        {
            if (itr == end_list) throw Error(k_exactly_two_msg);
            auto & out = **itr++;
            if (!string_to_number(beg, end, out)) {
                throw Error("MapObject points must be comprised of numeric "
                            "string tuples.");
            }
        });
        if (itr != end_list) throw Error(k_exactly_two_msg);
        points.push_back(v);
    });
    return points;
}

} // end of <anonymous> namespace
