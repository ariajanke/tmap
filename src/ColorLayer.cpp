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

#include "ColorLayer.hpp"

#include <SFML/Graphics.hpp>

namespace tmap {

ColorLayer::ColorLayer(): m_rect() {}

void ColorLayer::set_center(float x, float y) {
    m_rect.set_x(x - (m_rect.width() /2.f) + m_translation.x);
    m_rect.set_y(y - (m_rect.height()/2.f) + m_translation.y);
}

sf::Vector2f ColorLayer::center() const {
    return sf::Vector2f
        (m_rect.x() + m_rect.width() /2.f,
         m_rect.y() + m_rect.height()/2.f);
}

void ColorLayer::set_field_size(float w, float h) {
    sf::Vector2f cent = center();
    m_rect.set_width(w);
    m_rect.set_height(h);
    m_rect.set_position(cent.x - w/2.f, cent.y - h/2.f);
}

void ColorLayer::set_translation(float x, float y) {
    m_translation = sf::Vector2f(x, y);
    set_center(center().x, center().y);
}

void ColorLayer::set_color(sf::Color c)
    { m_rect.set_color(c); }

const std::string & ColorLayer::name() const {
    static const std::string t_name;
    return t_name;
}

/* protected virtual */ void ColorLayer::draw
    (sf::RenderTarget & target, sf::RenderStates) const
{
    return;
    target.draw(m_rect);
}

} // end of tmap namespace
