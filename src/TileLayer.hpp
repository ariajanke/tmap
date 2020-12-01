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

#include <common/Grid.hpp>

#include <tmap/TilePropertiesInterface.hpp>

#include "MapLayer.hpp"
#include "TiXmlHelpers.hpp"

#include <memory>
#include <type_traits>
#include <vector>

namespace sf { class View; }

namespace tmap {

class TileSet;
struct TileCellExposer;

/** A TileLayer is a Tiled Layer of the map, which any part of it can be drawn
 *  at any location at any size. TileLayers can be loaded from an XML element
 *  specified in a TilEd map file.
 *  @note Possible future feature: tile effects
 *  @warning A TileLayer is dependant on knowing constant addresses to tilesets
 *           so it is able to render tiles, see load_from_xml for more
 *           information.
 */
class TileLayer final : public MapLayer, public TilePropertiesInterface {
public:
    friend struct TileCellExposer;

    using ConstTileSetPtr = std::shared_ptr<const TileSet>;
    using TileSetPtr      = std::shared_ptr<TileSet>;

    /** A default TileLayer will not render any tiles, or anything for that
     *  matter.
     */
    TileLayer();

    void set_translation(float x, float y) override;

    /** Load a tile layer's XML, which comprises of it's tile matrix and
     *  meta-information. If any parse errors occur, an exception will be
     *  thrown (like all, inherits from std::exception).
     *  @param el XML element from TilEd in which TileLayer's information is
     *            defined.
     *  @param tilesets The complete and final set of tilesets for the map.
     *  @tparam Container should have elements that are constant TileSet STL
     *          shared pointer.
     *  @return Returns true if the xml was sucessfully loaded. (maybe removed)
     */
    template <typename Container>
    bool load_from_xml(const TiXmlElement * el, const Container & tilesets) {
        for (ConstTileSetPtr tileset : tilesets)
            m_tilesets.add_tileset(tileset);
        m_tilesets.sort();
        return load_from_xml(el);
    }

    /** A tile layer cannot know what tile size to use from the XML used to
     *  load a state.
     *  @note This function must be called before any rendering can be done.
     *  @param w width of all tiles in this layer
     *  @param h height of all tiles in this layer
     */
    void set_tile_size(float w, float h) { m_tile_size = sf::Vector2f(w, h); }

    /** Each TileLayer may or may not have a name. There is one special layer
     *  name 'ground' which is used by the owning Map object as the front-most
     *  background layer.
     *  @return Returns layer's name, "" means that it does not have a name.
     */
    const std::string & name() const override { return m_name; }

    const PropertyMap * operator () (int x, int y) const override;

    /** @copydoc TilePropertiesInterface::set_tile_gid(int,int,int) */
    void set_tile_gid(int x, int y, int new_gid) override;

    /** @copydoc TilePropertiesInterface::tile_gid(int,int) */
    int tile_gid(int x, int y) const override;

    /** @copydoc TilePropertiesInterface::width() */
    int width() const override;

    /** @copydoc TilePropertiesInterface::height() */
    int height() const override;

    /** @copydoc TilePropertiesInterface::tile_width() */
    float tile_width() const override
        { return m_tile_size.x; }

    /** @copydoc TilePropertiesInterface::tile_height() */
    float tile_height() const override
        { return m_tile_size.y; }

    /** @note exposed for testing purposes */
    static sf::IntRect compute_draw_range
        (const sf::View &, const sf::Vector2f & tilesize, int grid_width, int grid_height);

protected:
    void draw(sf::RenderTarget & target, sf::RenderStates) const override;

private:
    /** Special "adapter" class to, restricts usage of the tilesets container.
     *  Tilesets can be found and added quickly (O 1), but will have to be
     *  sorted before it can be used (an assert will fire if this step was
     *  neglected). @n
     *  @n
     *  Ordering of this container is the following:
     *  - TileSet 1: [first_gid end_gid)
     *  - TileSet 2: [first_gid end_gid)
     *  - TileSet 3: [first_gid end_gid)
     *  @n
     *  TileSet 2's first_gid should be equal or greater than TileSet 1's
     *  end_gid. Also, TileSet 2's first_gid should be equal or greater
     *  than 3's. And so on for any additional tileset(s).
     */
    class TileSetContainer {
    public:
        TileSetContainer() {}

        /** Adds tileset into the container, will need to be sorted later.
         *  @param tileset_ptr Pointer to new TileSet.
         */
        void add_tileset(ConstTileSetPtr tileset_ptr);

        /** Sorts container using std::sort */
        void sort();

        /** Finds the tileset that has a tile texture and rectangle for the
         *  given gid quickly (in O(log n)).
         *  @param gid Global ID number as defined by the TilEd map, where
         *             the TileLayer in this context is a member of.
         *  @return Pointer to tileset associated with the gid, if there is no
         *          such tileset in the container, nullptr is returned.
         */
        ConstTileSetPtr find_tileset_for_gid(int gid) const;

    private:
        bool m_is_sorted = false;
        std::vector<ConstTileSetPtr> m_tilesets;
    };

    struct TileCell {
        TileCell() {}
        explicit TileCell(int gid_): gid(gid_) {}
        int gid = 0;
        ConstTileSetPtr tset = nullptr;
    };

    TileCell & tile(int x, int y);

    const TileCell & tile(int x, int y) const;

    bool load_from_xml(const TiXmlElement * el);

    sf::IntRect compute_draw_range(const sf::View &) const;

    std::string m_name;
    Grid<TileCell> m_tile_matrix;
    sf::Vector2f m_tile_size;
    sf::Vector2f m_translation;
    int m_opacity = 1;

    TileSetContainer m_tilesets;
};

} // end of tmap namespace
