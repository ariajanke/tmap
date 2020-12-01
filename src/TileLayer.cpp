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

#include "TileLayer.hpp"
#include <tmap/Base64.hpp>
#include <tmap/ZLib.hpp>
#include "TileSet.hpp"

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <common/ConstString.hpp>
#include <common/StringUtil.hpp>

#include <tinyxml2.h>

#include <stdexcept>
#include <iostream>
#include <locale>
#include <cstdint>
#include <cmath>
#include <cassert>

namespace tmap {

struct TileCellExposer { using TileCell = TileLayer::TileCell; };

}

namespace {

using Error           = std::runtime_error              ;
using Int32           = std::int32_t                    ;
using TileSet         = tmap::TileSet                   ;
using ConstTileSetPtr = tmap::TileLayer::ConstTileSetPtr;
using TiXmlElement    = tmap::TiXmlElement              ;
using TileCell        = tmap::TileCellExposer::TileCell ;
using XmlRange        = tmap::XmlRange                  ;

/** Cleans out non-ascii and whitespace characters in the given string.
 *  @param str Target string to clean.
 */
void clean_string(std::string & str);

void load_tile_data_base64
    (const TiXmlElement * data_el, std::vector<TileCell> & loaded_tile_matrix,
     const char * data_text, int width, int height);

void load_tile_data_csv
    (std::vector<TileCell> & loaded_tile_matrix,
     const char * data_text, int width, int height);

void load_tile_data_xml
    (const TiXmlElement * data_el, std::vector<TileCell> & loaded_tile_matrix,
     const char * name, int width, int height);

} // end of <anonymous> namespace

namespace tmap {

TileLayer::TileLayer() {}

void TileLayer::set_translation(float x, float y)
    { m_translation = sf::Vector2f(x, y); }

const TileLayer::PropertyMap * TileLayer::operator ()
    (int x, int y) const /* override */
{
    const TileCell & tcell = tile(x, y);
    if (!tcell.tset) return nullptr;
    return tcell.tset->properties_on_gid(tcell.gid);
}

void TileLayer::set_tile_gid(int x, int y, int new_gid) {
    auto tset = m_tilesets.find_tileset_for_gid(new_gid);
    if (!tset) {
        throw Error("TileLayer::set_tile_gid: gid \"" + std::to_string(new_gid) +
                    "\" does not have a tileset associated with it. The map "
                    "file's text should specify which gid's map to which "
                    "tilesets.");
    }
    tile(x, y).gid  = new_gid;
    tile(x, y).tset = tset   ;
}

int TileLayer::tile_gid(int x, int y) const { return tile(x, y).gid; }

int TileLayer::width() const /* override */
    { return m_tile_matrix.width(); }

int TileLayer::height() const /* override */
    { return m_tile_matrix.height(); }

/* static */ sf::IntRect TileLayer::compute_draw_range
    (const sf::View & view, const sf::Vector2f & tilesize,
     int grid_width, int grid_height)
{
    // edge case tile size is zero
    if (tilesize == sf::Vector2f()) return sf::IntRect();

    auto field_size = view.getSize();
    // compute extreme points as reals (floats)
    float fx = float(view.getCenter().x - field_size.x / 2.f);
    float fy = float(view.getCenter().y - field_size.y / 2.f);

    sf::IntRect draw_range;
    // use float modulus to compute what the start tile is (convert to int)
    draw_range.left = int(std::floor(fx / tilesize.x));
    draw_range.top  = int(std::floor(fy / tilesize.y));

    // out of range
    if (draw_range.left >= grid_width || draw_range.top >= grid_height) {
        return sf::IntRect();
    }

    // compute draw offset, that is how much we need to "back up" tiles before
    // what we see makes sense
    sf::Vector2f offset(magnitude(std::remainder(fx, tilesize.x)),
                        magnitude(std::remainder(fy, tilesize.y)));

    // next is size of selection, make sure the entire screen is filled
    draw_range.width  = int(std::ceil((field_size.x + offset.x) / tilesize.x));
    draw_range.height = int(std::ceil((field_size.y + offset.y) / tilesize.y));

    // should now have tiles visible on screen

    if (draw_range.left < 0) {
        draw_range.width += draw_range.left;
        draw_range.left = 0;
    }
    if (draw_range.top < 0) {
        draw_range.height += draw_range.top;
        draw_range.top = 0;
    }

    // fix the range at the max if the right side/bottom side are out of range
    if (draw_range.left + draw_range.width > grid_width)
        draw_range.width = grid_width - draw_range.left;
    if (draw_range.top + draw_range.height > grid_height)
        draw_range.height = grid_height - draw_range.top;

    // draw limits (stop from attempting to render tiles out of the bounds of
    // the map)
    draw_range.width  = std::min(grid_width , draw_range.width );
    draw_range.height = std::min(grid_height, draw_range.height);
    return draw_range;
}

/* protected */ void TileLayer::draw
    (sf::RenderTarget & target, sf::RenderStates) const /* override */
{
    if (m_opacity == 0)
        return;
    sf::Sprite sprite_brush;

    DrawOnlyTarget restricted_target(&target);

    const sf::IntRect drange = compute_draw_range(target.getView());
    for (int y = drange.top ; y != drange.height + drange.top ; ++y) {
    for (int x = drange.left; x != drange.width  + drange.left; ++x) {
        const TileCell & tcell = tile(x, y);
        if (!tcell.tset) continue;
        sf::Vector2f loc(x*m_tile_size.x, y*m_tile_size.y);
        loc += m_translation;
        loc.x = std::floor(loc.x);
        loc.y = std::floor(loc.y);
        sprite_brush.setPosition(loc);
        sprite_brush.setColor(sf::Color(255, 255, 255, sf::Uint8(m_opacity)));
        sprite_brush.setTexture(tcell.tset->texture());

        auto frame = (*tcell.tset->tile_effect_for(tcell.gid))();
        sf::IntRect txt_rect;
        if (frame == TileFrame())
            txt_rect = tcell.tset->compute_texture_rect(tcell.gid);
        else
            txt_rect = tcell.tset->compute_texture_rect(frame);

        sprite_brush.setTextureRect(txt_rect);
        (*tcell.tset->tile_effect_for(tcell.gid))(sprite_brush, restricted_target);
        sprite_brush = sf::Sprite();
    }}
}

TileLayer::TileCell & TileLayer::tile(int x, int y)
    { return m_tile_matrix(x, y); }

const TileLayer::TileCell & TileLayer::tile(int x, int y) const
    { return m_tile_matrix(x, y); }

/* private */ bool TileLayer::load_from_xml(const TiXmlElement * el) {
    int width = read_int_attribute(el, "width");
    int height = read_int_attribute(el, "height");
    static constexpr const int k_max_color_value = 255;
    int opacity = k_max_color_value;
	if (el->Attribute("opacity")) {
        opacity = int(std::round
            (k_max_color_value*std::stof(el->Attribute("opacity"))));
	}
    const TiXmlElement * data_el = el->FirstChildElement("data");
    if (!data_el) {
        throw Error("Tile layer must contain a data tag.");
    }

    const char * name = el->Attribute("name");

    // now to read the tile matrix
    // the means of which are determined by its encoding and compression

    ConstString encoding;
    {
    const char * enc_cstr = data_el->Attribute("encoding");
    encoding = (enc_cstr ? enc_cstr : "");
    }
    const char * data_text = data_el->GetText();
    std::vector<TileCell> loaded_tile_matrix;

    loaded_tile_matrix.reserve(std::size_t(width*height));

    if (encoding == "base64" && data_text) {
        load_tile_data_base64
            (data_el, loaded_tile_matrix, data_text, width, height);
    } else if (encoding == "csv") {
        load_tile_data_csv(loaded_tile_matrix, data_text, width, height);
    } else if (encoding == "") {
        load_tile_data_xml(data_el, loaded_tile_matrix, name, width, height);
    } else {
        throw Error("tmap only knows how to handle base64 encoded, ZLib "
                    "compressed tile data, please change file to use this "
                    "format.");
    }

    for (TileCell & tcell : loaded_tile_matrix)
        tcell.tset = m_tilesets.find_tileset_for_gid(tcell.gid);

    Grid<TileCell> temp;
    temp.set_size(width, height);
    auto titr = loaded_tile_matrix.begin();

    // it's old and needs to be update (should be on desktop somewhere)
    for (auto & tc : temp) { tc = *titr++; }

    if (name) m_name = name;
    m_opacity = opacity;
    temp.swap(m_tile_matrix);
    return true;
}

/* private */ sf::IntRect TileLayer::compute_draw_range(const sf::View & view) const {
    return compute_draw_range(view, m_tile_size, width(), height());
}

// <--------------------- TileLayer::TileSetContainer ------------------------>

void TileLayer::TileSetContainer::add_tileset(ConstTileSetPtr tileset_ptr) {
    m_tilesets.push_back(tileset_ptr);
    m_is_sorted = false;
}

void TileLayer::TileSetContainer::sort() {
    std::sort(m_tilesets.begin(), m_tilesets.end(),
              [](ConstTileSetPtr lhs, ConstTileSetPtr rhs)
             { return lhs->begin_gid() < rhs->begin_gid(); });
    m_is_sorted = true;
}

TileLayer::ConstTileSetPtr TileLayer::TileSetContainer::find_tileset_for_gid
    (int gid) const
{
    // container must be sorted for binary search to work
    assert(m_is_sorted);

    if (gid == 0) return nullptr;

    // NTS: why are we implementing this by hand?
    auto beg = m_tilesets.begin();
    auto end = m_tilesets.end();

    while (beg != end) {
        auto itr = beg + ((end - beg)/2);
        ConstTileSetPtr cur = *itr;
        if (gid < cur->begin_gid()) // go left
            end = itr;
        else if (gid >= cur->end_gid()) // go right
            beg = itr;
        else
            return cur;
    }
    return nullptr;
}

} // end of tmap namespace

namespace {

inline bool is_comma(char c) { return c == ','; }
inline bool is_whitespace(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\n'; }

// 1st level of helpers

void clean_string(std::string & str) {
    if (str.empty()) return;
    const char k_erase_me = ' ' - 1;
    for (char & e : str) {
        if (e < ' ' || e > '~')
            e = k_erase_me;
    }
    str.erase(std::remove(str.begin(), str.end(), k_erase_me), str.end());
    while (std::isblank(str.back())) str.pop_back();
    auto itr = str.begin();
    while (std::isblank(*itr)) ++itr;
    str.erase(str.begin(), itr);
}

void load_tile_data_base64
    (const TiXmlElement * data_el, std::vector<TileCell> & loaded_tile_matrix,
     const char * data_text, int width, int height)
{
    // decode raw data from string in XML
    std::string filtered_data = data_text;
    clean_string(filtered_data);
    auto decoded_data = Base64::decode(filtered_data);

    // it can be ZLib compressed, so check
    const char * compression = data_el->Attribute("compression");

    // decompress if necessary
    if (compression && ConstString(compression) == "zlib")
        decoded_data = ZLib::decompress(decoded_data);

    if (int(decoded_data.size()/sizeof(Int32)) != width*height) {
        throw Error("Tile data does not provide information for all tiles "
                    "in the layer.");
    }

    const Int32 * id_data = reinterpret_cast<const Int32 *>(decoded_data.data());
    const Int32 * limit   = id_data + decoded_data.size()/sizeof(Int32);

    for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width ; ++x) {
        assert(limit != id_data); (void)limit;
        loaded_tile_matrix.push_back(TileCell { *id_data++ });
    }}
}

void load_tile_data_csv
    (std::vector<TileCell> & loaded_tile_matrix,
     const char * data_text, int width, int height)
{
    for_split<is_comma>(data_text, data_text + strlen(data_text),
        [&](const char * beg, const char * end)
    {
        trim<is_whitespace>(beg, end);
        loaded_tile_matrix.emplace_back();
        string_to_number(beg, end, loaded_tile_matrix.back().gid);
    });

    if (width*height != int(loaded_tile_matrix.size())) {
        throw Error("Number of tiles do not match size of tile sheet.");
    }
}

void load_tile_data_xml
    (const TiXmlElement * data_el, std::vector<TileCell> & loaded_tile_matrix,
     const char * name, int width, int height)
{
    if (!name)
        name = "<< NO NAME WAS GIVEN TO THIS TILE LAYER >>";

    int tile_counter = 0;
    for (const TiXmlElement & tile : XmlRange(data_el, "tile")) {
        constexpr auto XML_NO_ERROR = tinyxml2::XML_SUCCESS;
        int gid;
        ++tile_counter;
        if (tile_counter > width*height) {
            break;
        }
        if (tile.QueryIntAttribute("gid", &gid) != XML_NO_ERROR) {
            // need to have "warnings"
            throw Error("Tile tag must specify a gid attribute.");
        }
        loaded_tile_matrix.push_back(TileCell { gid });
    }
    if (tile_counter != width*height) {
        throw Error(
            std::string("Size of layer and number of tiles mismatch!\n") +
            "In layer \"" + name + "\" tiles found:" +
            std::to_string(tile_counter) + " (width: " +
            std::to_string(width) + " height: " + std::to_string(height) +
            ")");
    }
}

} // end of <anonymous> namespace
