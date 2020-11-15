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

#include <map>
#include <unordered_map>
#include <string>

namespace tmap {

/** Provides an interface to access the properties of any individual tile in
 *  the tile matrix that makes up a TileLayer. In an example, the ground layer
 *  for a TiledMap.
 */
class TilePropertiesInterface {
public:
    using PropertyMap = std::map<std::string, std::string>;

    /** Virtual destructor
     *  C++ note: Needed by derivative classes to ensure their destructors are
     *            called when this object is deleted from a base class pointer.
     */
    virtual ~TilePropertiesInterface();

    /** @param x X coordinate of the tile position (NOT pixel position)
     *  @param y Y coordinate of the tile position (NOT pixel position)
     *  @return Returns a pointer to an STL container with the properties of
     *          the requested tile, if the tile does not have any properties
     *          nullptr is returned.
     *          The pointer returned should point to an immutable container
     *          whose elements are also immutable.
     */
    virtual const PropertyMap * operator () (int x, int y) const = 0;

    /** Sets gid of a specific tile, good for changing the map at runtime.
     *  @param new_gid new global tile id
     *  @throw Will throw a std::runtime_error if the new_gid is not associated
     *         with any tileset in the loaded map.
     */
    virtual void set_tile_gid(int x, int y, int new_gid) = 0;

    /** @return Returns global id of the tile */
    virtual int tile_gid(int x, int y) const = 0;

    /** @return Returns the width of the tile matrix in tiles */
    virtual int width() const = 0;

    /** @return Returns the height of the tile matrix in tiles */
    virtual int height() const = 0;

    /** @return Returns width of ground tiles */
    virtual float tile_width() const = 0;

    /** @return Returns height of ground tiles */
    virtual float tile_height() const = 0;
};

} // end of tmap namespace
