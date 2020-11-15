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

namespace sf { class Texture; }

namespace tmap {

/** Tiled supports object layers. To take full advantage of that, this object
 *  defines fields for each of the Tiled built-in properties and an STL
 *  string -> string map for all the user custom properties
 */
struct MapObject {
    using PropertyMap = std::map<std::string, std::string>;
    using MapObjectContainer = std::vector<MapObject>;
    using PointVector = std::vector<sf::Vector2f>;
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
    
    /** Objects may also tiles, which will be linked to a texture */
    const sf::Texture * texture = nullptr;
    
    /** Objects that are tiles, will also have texture boundaries. */
    sf::IntRect texture_bounds;

    void swap(MapObject & rhs);
};

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
    std::swap(texture, rhs.texture);
    swap_rectangles(texture_bounds, rhs.texture_bounds);
}

} // end of tmap namespace
