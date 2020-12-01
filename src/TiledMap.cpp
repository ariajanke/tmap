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

#include <tmap/TiledMap.hpp>

#include "TiledMapImpl.hpp"

#include "MapLayer.hpp"

namespace {

using MapLayerIter      = tmap::TiledMap::MapLayerIter;
using MapLayerConstIter = tmap::TiledMap::MapLayerConstIter;
using TileSetPtr        = tmap::TiledMap::TileSetPtr;

} // end of <anonymous> namespace

namespace tmap {

TilePropertiesInterface::~TilePropertiesInterface() {}

void TiledMap::load_from_file(const char * filename)
    { m_impl->load_from_file(filename); }

void TiledMap::set_translation(const sf::Vector2f & offset)
    { m_impl->set_translation(offset); }

TileSetPtr TiledMap::get_tile_set_for_gid(int gid) const noexcept
    { return m_impl->get_tile_set_for_gid(gid); }

const TilePropertiesInterface * TiledMap::find_tile_layer
    (const std::string & name) const
{ return m_impl->find_tile_layer(name); }

TilePropertiesInterface * TiledMap::find_tile_layer(const std::string & name)
    { return m_impl->find_tile_layer(name); }

const TiledMap::PropertyMap & TiledMap::map_properties() const
    { return m_impl->map_properties(); }

const TiledMap::MapObjectContainer & TiledMap::map_objects() const
    { return m_impl->map_objects(); }

MapLayerIter TiledMap::begin()
    { return m_impl->begin(); }

MapLayerIter TiledMap::end()
    { return m_impl->end(); }

MapLayerConstIter TiledMap::begin() const {
    const TiledMapImpl & cmap = *m_impl;
    return cmap.begin();
}

MapLayerConstIter TiledMap::end() const{
    const TiledMapImpl & cmap = *m_impl;
    return cmap.end();
}

MapLayerIter TiledMap::find_layer
    (const std::string & name, MapLayerIter pos)
    { return m_impl->find_layer(name, pos); }

MapLayerIter TiledMap::find_layer
    (const char * name, MapLayerIter pos)
    { return m_impl->find_layer(name, pos); }

MapLayerConstIter TiledMap::find_layer
    (const std::string & name, MapLayerConstIter pos) const
    { return m_impl->find_layer(name, pos); }

MapLayerConstIter TiledMap::find_layer
    (const char * name, MapLayerConstIter pos) const
    { return m_impl->find_layer(name, pos); }

void TiledMap::swap(TiledMap & other)
    { std::swap(m_impl, other.m_impl); }

TiledMap::TiledMap():
    m_impl(new TiledMapImpl)
{}

TiledMap::TiledMap(TiledMap && rhs):
    m_impl(nullptr)
    { swap(rhs); }

TiledMap::~TiledMap()
    { delete m_impl; }

TiledMap & TiledMap::operator = (TiledMap && rhs) {
    swap(rhs);
    return *this;
}

/* private */ TiledMap::IterValuePair TiledMap::find_tile_effect_ref_and_name
    (const char * name, const IterValuePair & prev)
{ return m_impl->find_tile_effect_ref_and_name(name, prev); }

/* private */ TiledMap::IterValuePair TiledMap::find_tile_effect_ref_and_name
    (const char * name)
{
    return find_tile_effect_ref_and_name
           (name, TileEffectAssignmentPriv::k_start_iter);
}

} // end of tmap namespace
