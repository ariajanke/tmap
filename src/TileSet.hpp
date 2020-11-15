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

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <string>
#include <map>
#include <vector>

#include <tmap/TileEffect.hpp>

#include "TiledMapImpl.hpp"
#include "TiXmlHelpers.hpp"

namespace tmap {

class TileSet {
public:
    using IterValuePair = TiledMapImpl::IterValuePair;
    using ProperiesMap  = std::map<std::string, std::string>;

    TileSet();
    ~TileSet();

    // <------------------------- io interface ------------------------------->

    void set_referer(const std::string & referer);

    bool load_texture();

    void load_from_xml(const TiXmlElement * el);

    void set_tile_effect(const char * name, const char * value, TileEffect * te);

    IterValuePair find_tile_effect_ref_and_name
        (const char * name, IterValuePair prev);

    // <---------------------------- drawing --------------------------------->

    sf::IntRect compute_texture_rect(TileFrame frame) const;

    sf::IntRect compute_texture_rect(int gid) const;

    TileEffect * tile_effect_for(int gid) const;

    const sf::Texture & texture() const { return m_texture; }

    // <----------------------- tile set information ------------------------->

    int begin_gid() const;

    int end_gid() const;

    const ProperiesMap * properties_on_gid(int id) const;

private:

    void fix_file_path();

    // a "non-cached" version of (end_gid() - begin_gid())
    // This value is also derived from Tiled's XML, here derived from map
    // geometry
    sf::Vector2i size_in_tiles() const;

    void load_tile_properties(const TiXmlElement * tileset_el, int first_gid);

    void check_invarients() const;

    sf::Vector2i m_tile_size;
    sf::Vector2i m_image_size;
    std::string m_filename;
    int m_begin_gid;
    int m_end_gid;
    int m_spacing;
    sf::Texture m_texture;

    std::vector<ProperiesMap> m_properties;
    std::vector<TileEffect *> m_tile_effects;
    std::string m_referer;
};

} // end of tmap namespace
