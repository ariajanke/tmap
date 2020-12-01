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

#include <string>

namespace sf {
    class Sprite;
    class RenderTarget;
    class Drawable;
    class RenderStates;
}

namespace tmap {

class TileLayer;
class TileSet;
class DrawOnlyTarget;
class TileFrame;

struct TileFrameHasher {
    std::size_t operator() (const TileFrame &) const;
};

/** Represents a texture frame for a single tile on the map.
 *  @note Though the underlying type is an integer, the ID is not an arthimetic
 *        object semantically, therefore this class is defined in place of an
 *        integer. I hope also this relieves any temptation at directly
 *        changing the integer.
 */
class TileFrame {
public:
    TileFrame();
    bool operator == (const TileFrame &) const;
    bool operator != (const TileFrame &) const;
    std::size_t hash() const;
private:
    friend class TileSet;

    static TileFrame construct_privately(int);

    int m_gid;
};

/** A tile effect is a sprite transform applied onto a single tile. This
 *  provides an interface that every tile effect inherits from. @n
 *  @n
 *  To provide your own TileEffect, create a class which inherits from this
 *  one, then set which tiles (by name/value pairs) which will have that effect
 *  applied.
 *  @see TiledMap::access_tile_effect(const char*,const char*,TileEffect*)
 *  on how to do this.
 */
struct TileEffect {

    virtual ~TileEffect();

    /** This is called to render the tile by the parent tile layer.
     *  @param spt    The tile as an SFML sprite, which can be modified to the
     *                liking of the client.
     *  @param target The SFML render target, held behind a restricted
     *                interface that can only be used for drawing.
     *  @see DrawOnlyTarget
     */
    virtual void operator () (sf::Sprite & spt, DrawOnlyTarget & target) = 0;

    virtual TileFrame operator () () const;
};

/** For tile effects, no effect is desired most of the time. So this built-in
 *  TileEffect is present to serve that purpose. When the functor is applied to
 *  a tile, it will simply draw that tile. @n
 *  For convenience's sake, this object is a stateless singleton. @n
 *  @note TiledMap requires a pointer to the tile effect object, therefore a
 *  singleton rather than static class.
 */
class NoTileEffect final : public TileEffect {
public:

    /** Does not modify the tile sprite what so ever, and simply draws it.
     *  @param spt given tile sprite
     *  @param target render target
     *  @see DrawOnlyTarget
     */
    void operator () (sf::Sprite & spt, DrawOnlyTarget & target) override final;

    /** @return Returns the one and only instance. */
    static NoTileEffect & instance();

private:

    NoTileEffect() {}
};

/** The DrawOnlyTarget class, takes the place of sf::RenderTarget. Passing such
 *  a reference to a single tile effect feels like breaking encapsulation to
 *  me. So this restricted interface is meant to preserve it (by only exposing
 *  the draw method).
 */
class DrawOnlyTarget {
public:

    DrawOnlyTarget(const DrawOnlyTarget &) = delete;
    DrawOnlyTarget(DrawOnlyTarget &&) = delete;
    DrawOnlyTarget & operator = (const DrawOnlyTarget &) = delete;
    DrawOnlyTarget & operator = (DrawOnlyTarget &&) = delete;

    /** This function is equivalent to sf::RenderTarget::draw.
     *  @see sf::RenderTarget::draw(const Drawable&, const RenderStates&)
     */
    void draw(const sf::Drawable & drawable, const sf::RenderStates & states);

    /** This function is equivalent to sf::RenderTarget::draw, where states
     *  will be set to RenderStates::Default.
     *  @see sf::RenderTarget::draw(const Drawable&, const RenderStates&)
     */
    void draw(const sf::Drawable & drawable);

private:
    friend class TileLayer;

    DrawOnlyTarget(sf::RenderTarget *);

    sf::RenderTarget * m_target;
};

// <------------------------- IMPLEMENTATION DETAIL -------------------------->

class TiledMap;
class TiledMapImpl;

class TileEffectAssignmentPriv {
    friend class TiledMap;
    friend class TiledMapImpl;

    struct IterValuePair {
        IterValuePair();
        IterValuePair(const std::string * v_, TileEffect ** te_);

        const std::string * value;
        TileEffect ** tile_effect;
        int tile_set_index;
        TileFrame tile_frame;
    };
    static const IterValuePair k_start_iter;
};

} // end of tmap namespace
