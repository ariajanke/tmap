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

#include <tmap/TilePropertiesInterface.hpp>
#include <tmap/TileEffect.hpp>
#include <tmap/MapObject.hpp>
#include <tmap/TiledMap.hpp>

#include "TiXmlHelpers.hpp"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace sf {
    class RenderTarget;
    class View;
}

namespace tmap {

class TileSet;
class MapLayer;

class TiledMapImpl {
public:
    using PropertyMap        = std::map<std::string, std::string>;
    using MapObjectContainer = MapObject::MapObjectContainer;
    using MapLayerIter       = TiledMap::MapLayerIter;
    using MapLayerConstIter  = TiledMap::MapLayerConstIter;
    using TileSetPtr        = std::shared_ptr<tmap::TileSet>;
    using TileSetPtrVector  = std::vector<TileSetPtr>;

    TiledMapImpl();

    ~TiledMapImpl();

    void load_from_file(const char * filename);

    void apply_view(const sf::View & view);

    void set_translation(const sf::Vector2f & offset);

    void assign_tile_effect_with_property_pair
        (const char * name, const char * value, TileEffect * te);

    const TilePropertiesInterface * find_tile_layer(const std::string & name) const;

    TilePropertiesInterface * find_tile_layer(const std::string & name);

    const PropertyMap & map_properties() const;

    const MapObjectContainer & map_objects() const;

    MapLayerIter begin();

    MapLayerIter end();

    MapLayerConstIter begin() const;

    MapLayerConstIter end() const;

    MapLayerIter find_layer
        (const std::string & name, MapLayerIter pos = MapLayerIter());

    MapLayerIter find_layer
        (const char * name, MapLayerIter pos = MapLayerIter());

    MapLayerConstIter find_layer
        (const std::string & name                   ,
         MapLayerConstIter pos = MapLayerConstIter()) const;

    MapLayerConstIter find_layer
        (const char * name, MapLayerConstIter pos = MapLayerConstIter()) const;

    using IterValuePair = TileEffectAssignmentPriv::IterValuePair;

    IterValuePair find_tile_effect_ref_and_name
        (const char * name, const IterValuePair & prev);

private:

    using MapLayerContainer = std::vector<std::unique_ptr<MapLayer>>;
    using MapLayerMap = std::unordered_multimap<std::string, typename TiledMap::MapLayerIter>;
    using TileLayerMap = std::unordered_map<std::string, TilePropertiesInterface *>;

    TiledMapImpl(const TiledMapImpl &) = delete;
    TiledMapImpl(TiledMapImpl &&) = delete;
    TiledMapImpl & operator = (const TiledMapImpl &) = delete;
    TiledMapImpl & operator = (TiledMapImpl &&) = delete;

    void load_map_objects(const TiXmlElement * map_el, const TileSetPtrVector &);

    template <bool k_tf_val, typename A, typename B>
    struct TypeSelect { using Type = A; };

    template <typename A, typename B>
    struct TypeSelect<false, A, B> { using Type = B; };

    template <bool k_is_const>
    using LayerIterTypeSelect = typename TypeSelect<k_is_const, TiledMap::MapLayerConstIter, TiledMap::MapLayerIter>::Type;

    template <bool k_is_const>
    using NameMapIterTypeSelect = typename TypeSelect<k_is_const, MapLayerMap::const_iterator, MapLayerMap::iterator>::Type;

    template <bool k_is_const>
    using NameRangePair = std::pair<NameMapIterTypeSelect<k_is_const>, NameMapIterTypeSelect<k_is_const>>;

    template <bool k_is_const>
    static LayerIterTypeSelect<k_is_const>
        find_lowest_above
        (NameRangePair<k_is_const> range, LayerIterTypeSelect<k_is_const> pos, LayerIterTypeSelect<k_is_const> end)
    {
        auto rv = end;
        if (std::distance(range.first, range.second) == 0) return rv;
        for (auto itr = range.first; itr != range.second; ++itr) {
            if (pos <= itr->second && itr->second < rv) {
                rv = itr->second;
            }
        }
        return rv;
    }

    int m_map_width;
    int m_map_height;
    int m_tile_width;
    int m_tile_height;

    MapLayerContainer m_layers;
    TiledMap::MapLayerContainer m_drawable_layers;
    MapLayerMap m_name_to_draw_layer;
    // pointers contained should live as long as this class instance
    TileLayerMap m_name_to_tile_layer;

    TilePropertiesInterface * m_ground_layer;
    PropertyMap m_whole_map_properties;

    MapObjectContainer m_map_objects;
    TileSetPtrVector m_tile_sets;

};

} // end of tmap namespace
