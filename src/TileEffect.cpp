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

#include <tmap/TileEffect.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace tmap {

std::size_t TileFrameHasher::operator() (const TileFrame & rhs) const
    { return rhs.hash(); }

TileFrame::TileFrame(): m_gid(-2) {}

bool TileFrame::operator == (const TileFrame & rhs) const
    { return m_gid == rhs.m_gid; }

bool TileFrame::operator != (const TileFrame & rhs) const
    { return m_gid != rhs.m_gid; }

std::size_t TileFrame::hash() const {
    return std::size_t(m_gid * 8599) ^ std::size_t(0x4d1b66b694a4734c);
}

/* static private */ TileFrame TileFrame::construct_privately(int gid) {
    TileFrame rv;
    rv.m_gid = gid;
    return rv;
}

TileFrame TileEffect::operator () () const { return TileFrame(); }

void DrawOnlyTarget::draw
    (const sf::Drawable & drawable, const sf::RenderStates & states)
{ m_target->draw(drawable, states); }

void DrawOnlyTarget::draw(const sf::Drawable & drawable)
    { m_target->draw(drawable, sf::RenderStates::Default); }

/* private */ DrawOnlyTarget::DrawOnlyTarget(sf::RenderTarget * target_):
    m_target(target_)
{}

TileEffect::~TileEffect(){}

void NoTileEffect::operator () (sf::Sprite & spt, DrawOnlyTarget & target)
    { target.draw(spt); }

NoTileEffect & NoTileEffect::instance() {
    static NoTileEffect inst;
    return inst;
}

/* private static */ const TileEffectAssignmentPriv::IterValuePair
    TileEffectAssignmentPriv::k_start_iter;

TileEffectAssignmentPriv::IterValuePair::IterValuePair():
    value(nullptr), tile_effect(nullptr), tile_set_index(-1) {}

TileEffectAssignmentPriv::IterValuePair::IterValuePair
    (const std::string * v_, TileEffect ** te_):
    value(v_), tile_effect(te_), tile_set_index(-1)
{}

} // end of tmap namespace
