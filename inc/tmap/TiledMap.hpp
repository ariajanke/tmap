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
#include <functional>

#include <tmap/MapObject.hpp>
#include <tmap/TileEffect.hpp>
#include <tmap/TilePropertiesInterface.hpp>

// forwards for SFML
namespace sf {
    class RenderTarget;
    class Drawable;
    class View;
}

namespace tmap {

class TiledMapImpl;

/** The main interface for loading and accessing information from a Tiled map.
 *
 *  @note This class uses the "impl" pattern, so any source code using this
 *        class will not need to know anything about the dependancies. @n
 *        (being a Base64 codec, Zlib and tinyxml2)
 *
 *  @note Exception Safety: the basic guarentee is made for all functions
 *
 *  @note TiledMap will not attempt to render tiles outside its view as
 *        determined by calls to set_center and set_field_size
 *
 *  Features: @n
 *  - maybe drawn as several layers using SFML
 *  - provide an STL interface into tile information, map properties and
 *    objects (loaded from object layers)
 *  - Objects from object layers are all loaded into a map objects container
 *    accessible from the TiledMap interface
 *  - supports tile encoding for base64 and base64 + Zlib + CSV + plain XML
 *  - layers can be iterated using thier names as bounds, layers may only be
 *    drawn, not modified
 *  - tile effects, any tile in a tileset (not individual tiles in a map)
 *    can have additional work done on the sprites and rendered multiple times
 *    allowing for graphical effects
 *
 *  Usage: @n
 *  @n
 *  When loading: @n
 *  If any problems occur with the loading/parsing process; the loading
 *  function will throw an exception, so the client code will need to catch it.
 *  @n
 *  (maybe in the future an "insensitive" mode can be offered, that will ignore
 *   as many errors as possible) @n
 *  @n
 *  [OBSOLETE] When rendering: @n
 *  To use this class both "field size" and center must be specified, this
 *  determines which tiles are rendered. Rendering a large map with thousands
 *  of tiles, may consume a lot of CPU and cause the program to lose frames/
 *  operate slowly. @n
 *  @n
 *  Restriction: this map cannot be copied, you can however swap contents
 *  with the 'swap' method. @n
 *  It is possible to treat this object like unique_ptr
 */
class TiledMap {
public:
    using PropertyMap        = std::map<std::string, std::string>;
    using MapObjectContainer = MapObject::MapObjectContainer;
    using MapLayerContainer  = std::vector<const sf::Drawable *>;
    using MapLayerIter       = MapLayerContainer::iterator;
    using MapLayerConstIter  = MapLayerContainer::const_iterator;
    using TileSetPtr         = MapObject::TileSetPtr;

    TiledMap();
    TiledMap(const TiledMap &) = delete;
    TiledMap(TiledMap &&);
    ~TiledMap();

    TiledMap & operator = (const TiledMap &) = delete;
    TiledMap & operator = (TiledMap &&);

    /** Loads an Tiled XML Map from the given filename
     *  The function is sensitive to XML errors.
     *  @throw Throws an exception if any error occurs while parsing the XML.
     *         The exception (std::runtime_error) will contain a human readable
     *         string detailing the error. @n
     *         Strong guarantee
     *  @param filename file path specifying map file's location
     */
    void load_from_file(const char * filename);

    //! @copydoc TiledMap::load_from_file(const char*)
    void load_from_file(const std::string &);

    /** Sets the amount which tile rendering will be offset.
     *  @param offset offset vector to displace rendering
     */
    void set_translation(const sf::Vector2f & offset);

    /** Accesses all tiles in all tile sets that has the given attribute, and
     *  allows client code to set 'tile effect' pointers.
     *  @param attribute     given attribute string used to match all tiles
     *                       (case sensitive)
     *  @param val_teffect_f callable function/functor with the following
     *                       signature: @n
     *                       void val_teffect_f(const std::string & value,
     *                                          tmap::TileEffect *& teffct) @n
     *                       parameters:
     *                       - value  : value string of the tile
     *                       - teffect: assignable pointer, set to the desired
     *                                  tile effect pointer
     *  @note Client code is responsible for managing the lifetime of any tile
     *        effects. TiledMap expects that the tile effect pointers remain
     *        at a constant address for thier lifetimes.
     *  @note Dev note: extensive documentation implies that this maybe too
     *        complex an interface, perhaps I could simplify it somehow?
     *  @see tmap::TileEffect
     */
    template <typename Func>
    void access_tile_effect(const char * attribute, Func && val_teffect_f);

    /** @brief Gets a tile set interface pointer for some given gid.
     *  @param gid global tile id
     *  @returns pointer the tile set associated with the given gid, nullptr
     *           if gid either is out of range to the tile map or is "k_no_tile"
     *           (Note: TilEd defines this as gid == 0)
     */
    TileSetPtr get_tile_set_for_gid(int gid) const noexcept;

    /** Finds a tile layer by name, if the name's are amibigiuous, then only
     *  the first layer with that name is returned.
     *  @param  name Name of the layer to find.
     *  @return Returns nullptr if the layer is not found, otherwise a pointer
     *          to that layer is returned.
     */
    const TilePropertiesInterface * find_tile_layer(const std::string & name) const;

    /** Finds a tile layer by name, if the name's are amibigiuous, then only
     *  the first layer with that name is returned. This writable layer allows
     *  changing tile ids at runtime.
     *  @param  name Name of the layer to find.
     *  @return Returns nullptr if the layer is not found, otherwise a pointer
     *          to that layer is returned.
     */
    TilePropertiesInterface * find_tile_layer(const std::string & name);

    /** @return Returns map-wide properties as an STL map of string -> string.
     */
    const PropertyMap & map_properties() const;

    /** @return Returns a container (constant reference) with all objects that
     *          were found in object layers.
     */
    const MapObjectContainer & map_objects() const;

    /** @brief Forward iterator, refering to the first map layer.
     *
     *  TiledMap offers an interface to iterate the map layers without exposing
     *  the implementation. So conversion methods are available to help draw
     *  the layers via their iterators.
     *
     *  @return Returns the aforementioned iterator
     */
    MapLayerIter begin();

    /** @see begin()
     *  @return Returns a Forward iterator, refering to "one past the last"
     *          iterator. Consistent in meaning with the STL.
     */
    MapLayerIter end();

    /** @see begin()
     *  @return Returns a forward iterator, refering to the first map layer.
     */
    MapLayerConstIter begin() const;

    /** @see begin()
     *  @return Returns a Forward iterator, refering to "one past the last"
     *          iterator. Consistent in meaning with the STL.
     */
    MapLayerConstIter end() const;

    /** @brief Finds a MapLayer by name.
     *
     *  Searchs the TiledMap layers starting from pos (or from the result of
     *  begin(), if no iterator is provided) until end()
     *
     *  is reached. If a layer
     *  with the given name is found, it is returned as an iterator. @n
     *  @n
     *  Constant versions of this function, behave identically, returning
     *  constant iterators instead. @n
     *  @n
     *  Complexity: O(n), where n is the number of map layers.
     *
     *  @param name Name of the MapLayer
     *  @param pos  Iterator where to begin the search (must be a member of
     *              this map)
     *  @return Returns an iterator refering the found layer or the result of
     *          end() if the search fails.
     */
    MapLayerIter find_layer
        (const std::string & name, MapLayerIter pos = MapLayerIter());

    /** @copydoc find_layer(const std::string&,MapLayerIter) */
    MapLayerIter find_layer
        (const char * name, MapLayerIter pos = MapLayerIter());

    /** @copydoc find_layer(const std::string&,MapLayerIter) */
    MapLayerConstIter find_layer
        (const std::string & name                   ,
         MapLayerConstIter pos = MapLayerConstIter()) const;

    /** @copydoc find_layer(const std::string&,MapLayerIter) */
    MapLayerConstIter find_layer
        (const char * name, MapLayerConstIter pos = MapLayerConstIter()) const;

    /** Swap the entire map's contents with another.
     *  @param other the other map to swap contents with
     */
    void swap(TiledMap & other);

private:
    using IterValuePair = TileEffectAssignmentPriv::IterValuePair;

    // need a way to differenciate between beginning and ending iterator
    IterValuePair find_tile_effect_ref_and_name
        (const char * name, const IterValuePair & prev);

    IterValuePair find_tile_effect_ref_and_name(const char * name);

    TiledMapImpl * m_impl = nullptr;
};

template <typename Func>
void TiledMap::access_tile_effect(const char * attribute, Func && val_teffect_f) {
    IterValuePair itr_and_value = find_tile_effect_ref_and_name(attribute);
    while (itr_and_value.value) {
        val_teffect_f(std::cref(*itr_and_value.value)     ,
                      std::ref(*itr_and_value.tile_effect),
                      itr_and_value.tile_frame            );
        itr_and_value = find_tile_effect_ref_and_name(attribute, itr_and_value);
    }
}

inline void TiledMap::load_from_file(const std::string & filename) {
    load_from_file(filename.c_str());
}

} // end of tmap namespace
