#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include <tmap/TiledMap.hpp>

#include "../src/TileLayer.hpp"

#include <common/ParseOptions.hpp>
#include <common/StringUtil.hpp>
#include <common/TestSuite.hpp>

#include <cassert>

namespace {

class Diamond final : public sf::Drawable {
public:
    explicit Diamond(const tmap::MapObject &);

    void update(double et);

private:
    static bool is_colon(char c) { return c == ':'; }
    static bool is_comma(char c) { return c == ','; }
    static bool is_space(char c) { return c == ' '; }

    void draw(sf::RenderTarget &, sf::RenderStates) const override;

    static constexpr const double k_frame_max = 0.4;
    double m_frame_time = 0.;
    std::vector<int> m_frames;
    std::vector<int>::const_iterator m_current_frame;
    tmap::MapObject::TileSetPtr m_tileset;
    sf::Vector2f m_location;
};

void do_draw_range_tests();

} // end of <anonymous> namespace

int main(int argc, char ** argv) {
    static constexpr const int   k_fps        =  30  ;
    static constexpr const int   k_width      = 320  ;
    static constexpr const int   k_height     = 240  ;
    static constexpr const float k_move_speed =  50.f;

    do_draw_range_tests();

    sf::RenderWindow window;
    window.create(sf::VideoMode(k_width*2, k_height*2), "Demo Map viewer");
    window.setFramerateLimit(k_fps);

    {
    auto view = window.getView();
    view.setSize  (float(k_width)     , float(k_height)     );
    view.setCenter(float(k_width)*0.5f, float(k_height)*0.5f);
    window.setView(view);
    }

    std::string chosen_map = "test-map.tmx";
    if (argc > 1) {
        chosen_map = argv[1];
    }
    tmap::TiledMap test_map;
    std::vector<sf::Sprite> sprites;
    std::vector<Diamond> diamonds;
    test_map.load_from_file(chosen_map);
    for (const auto & obj : test_map.map_objects()) {
        using Mo = tmap::MapObject;

        // these tests depend on the data present in the test map
        switch (obj.shape_type) {
        case Mo::k_polyline: assert(obj.points.size() == 3); break;
        case Mo::k_polygon : assert(obj.points.size() == 5); break;
        default: break;
        }
        if (obj.type == "diamond") {
            diamonds.emplace_back(obj);
            continue;
        }
        if (!obj.tile_set) continue;
        sf::Sprite spt;
        spt.setTexture(obj.tile_set->texture());
        auto texture_bounds = obj.tile_set->texture_rectangle(obj.local_tile_id);
        spt.setTextureRect(texture_bounds);
        spt.setPosition(obj.bounds.left, obj.bounds.top);
        spt.setScale(obj.bounds.width  / float(texture_bounds.width ),
                     obj.bounds.height / float(texture_bounds.height));
        sprites.push_back(spt);
    }

    while (window.isOpen()) {
        {
        sf::Event event;
        sf::Vector2f view_delta;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::KeyPressed:
                break;
            case sf::Event::KeyReleased:
                switch (event.key.code) {
                case sf::Keyboard::Escape: window.close(); break;
                case sf::Keyboard::W: view_delta.y -= 1.f; break;
                case sf::Keyboard::A: view_delta.x -= 1.f; break;
                case sf::Keyboard::S: view_delta.y += 1.f; break;
                case sf::Keyboard::D: view_delta.x += 1.f; break;
                default: break;
                }
                break;
            case sf::Event::Closed: window.close(); break;
            default: break;
            }
        }
        if (view_delta != sf::Vector2f()) {
            view_delta = normalize(view_delta)*k_move_speed;
            auto view = window.getView();
            view.move(view_delta);
            window.setView(view);
        }
        }
        for (auto & diamond : diamonds) {
            diamond.update(1. / double(k_fps));
        }
        window.clear();
        for (auto & layer : test_map) {
            window.draw(*layer);
        }
        for (auto & spt : sprites) {
            window.draw(spt);
        }
        for (const auto & diamond : diamonds) {
            window.draw(diamond);
        }
        window.display();
    }
}

namespace {

Diamond::Diamond(const tmap::MapObject & obj):
    m_tileset(obj.tile_set),
    m_location(obj.bounds.left, obj.bounds.top)
{
    const auto * props = m_tileset->properties_on(obj.local_tile_id);
    if (!props) return;
    auto itr = props->find("on-collection");
    if (itr == props->end()) return;
    const char * beg = &itr->second.front();
    const char * end = beg + itr->second.size();
    m_frames.push_back(obj.local_tile_id);
    int num = 0;
    for_split<is_colon>(beg, end, [&num, this](const char * beg, const char * end) {
        if (num != 1) {
           ++num;
            return;
        }
        for_split<is_comma>(beg, end, [this](const char * beg, const char * end) {
            trim<is_space>(beg, end);
            int num = 0;
            if (string_to_number(beg, end, num)) { m_frames.push_back(num); }
        });
        ++num;
    });
    m_current_frame = m_frames.begin();
}

void Diamond::update(double et) {
    if ( (m_frame_time += et) < k_frame_max ) {
        return;
    }
    m_frame_time = 0.;
    if (++m_current_frame == m_frames.end()) {
        m_current_frame = m_frames.begin();
    }
}

/* private */ void Diamond::draw(sf::RenderTarget & target, sf::RenderStates states) const {
    sf::Sprite brush;
    brush.setTexture(m_tileset->texture());
    brush.setTextureRect(m_tileset->texture_rectangle(*m_current_frame));
    brush.setPosition(m_location);
    target.draw(brush, states);
}

void do_draw_range_tests() {
    static const constexpr int k_width       = 320;
    static const constexpr int k_height      = 240;
    static const constexpr int k_tile_size   =  16;
    static const constexpr int k_grid_width  =  10; // 160 px
    static const constexpr int k_grid_height =   8; // 128 px

    static_assert(k_width  % k_tile_size == 0, "");
    static_assert(k_height % k_tile_size == 0, "");
    static const constexpr int k_max_width_tile  = k_width  / k_tile_size;
    static const constexpr int k_max_height_tile = k_height / k_tile_size;

    static auto offset_creates = [](int x, int y, sf::IntRect drange) {
        auto xt = x / k_tile_size;
        sf::View view(sf::Vector2f( float(x + k_width / 2), float(y + k_height / 2) ),
                      sf::Vector2f( float(    k_width    ), float(    k_height    ) ));
        auto tileszv = sf::Vector2f(float(k_tile_size), float(k_tile_size));
        auto gv = tmap::TileLayer::compute_draw_range(
            view, tileszv, k_grid_width, k_grid_height);
        return gv == drange;
    };

    using sf::IntRect;

    ts::TestSuite suite;
    suite.start_series("compute_draw_range");
    suite.test([]() {
        return ts::test(offset_creates(
            0, 0, IntRect(0, 0, k_grid_width, k_grid_height)));
    });
    suite.test([]() {
        return ts::test(offset_creates(
            (-k_max_width_tile + k_grid_width / 2)*k_tile_size, 0,
            IntRect(0, 0, k_grid_width / 2, k_grid_height)));
    });
    suite.test([]() {
        return ts::test(offset_creates(
            0, (-k_max_height_tile + k_grid_height / 2),
            IntRect(0, 0, k_grid_width, k_grid_height / 2)));
    });
    suite.test([]() {
        return ts::test(offset_creates(
            (k_grid_width / 2)*k_tile_size, 0,
            IntRect(k_grid_width / 2, 0, k_grid_width - (k_grid_width / 2), k_grid_height)));
    });
    suite.test([]() {
        return ts::test(offset_creates(
            0, (k_grid_height / 2)*k_tile_size,
            IntRect(0, k_grid_height / 2, k_grid_width, k_grid_height / 2)));
    });
}

} // end of <anonymous> namespace
