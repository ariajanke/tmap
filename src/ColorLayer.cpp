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

ColorLayer::ColorLayer() {}

void ColorLayer::set_translation(float, float)
    {}

void ColorLayer::set_color(sf::Color c)
    { m_color = c; }

const std::string & ColorLayer::name() const {
    static const std::string t_name;
    return t_name;
}

/* protected virtual */ void ColorLayer::draw
    (sf::RenderTarget & target, sf::RenderStates states) const
{
    DrawRectangle drect;
    auto view = target.getView();
    drect.set_position(view.getCenter().x - view.getSize().x*0.5f,
                       view.getCenter().y - view.getSize().y*0.5f);
    drect.set_size(view.getSize().x, view.getSize().y);
    drect.set_color(m_color);
    target.draw(drect, states);
}

} // end of tmap namespace
