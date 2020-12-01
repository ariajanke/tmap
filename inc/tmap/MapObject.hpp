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

#include <SFML/Graphics/Rect.hpp>

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace sf { class Texture; }

namespace tmap {

struct TileEffect;

/** @brief Provides some basic information about a tile set
 *
 *  (anchored in TileSet.cpp)
 */
struct TileSetInterface {
    using PropertyMap = std::map<std::string, std::string>;

    static constexpr const int k_no_tile = 0;

    virtual ~TileSetInterface();

    /** Converts local tileset id to a global tile id.
     *  @note Some tilesets may have properties which refer to other tiles by
     *        a local id.
     *  @throws if the tid is not owned by the tile set or is otherwise invalid
     *  @param tid local tileset id
     *  @return returns global id
     */
    virtual int convert_to_gid(int tid) const = 0;

    /** Converts global id to local id.
     *  @param gid global id
     *  @returns local id, or k_no_tile if gid is not owned by this tile set
     */
    virtual int convert_to_local_id(int gid) const = 0;

    /** @throws if for some reason that the tile set does not have a texture
     *          (this should only occur if there's a problem with THIS library)
     *  @return const reference to the tileset's texture
     */
    virtual const sf::Texture & texture() const = 0;

    /** @param gid global id (NOT local id, you may need to convert)
     *  @throws if the specified tid does not belong to the tileset
     *  @return returns the texture rectangle
     */
    virtual sf::IntRect texture_rectangle(int tid) const = 0;

    /** @param gid global tile id
     *  @returns the tile effect associated with a global id, nullptr is
     *           returned if the tileset does not contain the gid
     */
    virtual TileEffect * get_effect(int tid) const = 0;

    /** @param tid local tile set id
     *  @returns property pairs for a given tid
     */
    virtual const PropertyMap * properties_on(int tid) const = 0;

    /** @param tid local tile set id
     *  @returns the type attribute for the local tile id
     */
    virtual const std::string & type_of(int tid) const = 0;
};

// ----------------------------------------------------------------------------

/** Tiled supports object layers. To take full advantage of that, this object
 *  defines fields for each of the Tiled built-in properties and an STL
 *  string -> string map for all the user custom properties
 */
struct MapObject {
    using PropertyMap        = TileSetInterface::PropertyMap;
    using MapObjectContainer = std::vector<MapObject>;
    using PointVector        = std::vector<sf::Vector2f>;
    using TileSetPtr         = std::shared_ptr<const TileSetInterface>;

    static constexpr const int k_no_tile = TileSetInterface::k_no_tile;

    enum ShapeType {
        k_rectangle, k_text, k_ellipse, k_polygon, k_polyline, k_invalid_shape
    };

    /** string value of the "name" attribute */
    std::string name;

    /** string value of the "type" attribute */
    std::string type;

    /** Bounds of the object determined by the attributes "x", "y", "width"
     *  and "height" */
    sf::Rect<float> bounds;

    /** All other attributes in the object tag as name and value pairs */
    PropertyMap custom_properties;

    /** Objects may have bounds defined by a set of points, rather than width
     *  and height */
    PointVector points;

    /** Objects may take many shapes, depends on the tool used to create them */
    ShapeType shape_type = k_invalid_shape;

    /** Objects may also tiles, which will have a local tileset id */
    int local_tile_id = 0;

    /** Objects may also tiles, this tile set interface (which will be
     *  non-null if it is a tile object), provides methods to access some basic
     *  information about the tile.
     *  @note Only tile objects will have a non-null tile set
     *  @see TileSetInterface
     */
    TileSetPtr tile_set = nullptr;

    void swap(MapObject & rhs);
};

// ----------------------------------------------------------------------------

template <typename T>
void swap_rectangles(sf::Rect<T> & lhs, sf::Rect<T> & rhs) {
    std::swap(lhs.left  , rhs.left  );
    std::swap(lhs.top   , rhs.top   );
    std::swap(lhs.width , rhs.width );
    std::swap(lhs.height, rhs.height);
}

inline void MapObject::swap(MapObject & rhs) {
    name.swap(rhs.name);
    type.swap(rhs.type);
    swap_rectangles(bounds, rhs.bounds);
    custom_properties.swap(rhs.custom_properties);
    points.swap(rhs.points);
    std::swap(shape_type, rhs.shape_type);
    std::swap(local_tile_id, rhs.local_tile_id);
    tile_set.swap(rhs.tile_set);
}

} // end of tmap namespace
