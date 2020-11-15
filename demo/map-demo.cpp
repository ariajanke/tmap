#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include <tmap/TiledMap.hpp>

#include <common/ParseOptions.hpp>

#include <cassert>

int main(int argc, char ** argv) {
    sf::RenderWindow window;
    window.create(sf::VideoMode(640, 480), "Demo Map viewer");
    window.setFramerateLimit(30);
    
    std::string chosen_map = "test-map.tmx";
    if (argc > 1) {
        chosen_map = argv[1];
    }
    tmap::TiledMap test_map;
    std::vector<sf::Sprite> sprites;
    test_map.load_from_file(chosen_map);
    for (const auto & obj : test_map.map_objects()) {
        using Mo = tmap::MapObject;
        if (!obj.texture) continue;
        // these tests depend on the data present in the test map
        switch (obj.shape_type) {
        case Mo::k_polyline: assert(obj.points.size() == 3); break;
        case Mo::k_polygon : assert(obj.points.size() == 5); break;
        default: break;
        }
        sf::Sprite spt;
        spt.setTexture(*obj.texture);
        spt.setTextureRect(obj.texture_bounds);
        spt.setPosition(obj.bounds.left, obj.bounds.top);
        spt.setScale(obj.bounds.width  / float(obj.texture_bounds.width ),
                     obj.bounds.height / float(obj.texture_bounds.height));
        sprites.push_back(spt);
    }

    while (window.isOpen()) {
        {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            
            case sf::Event::Closed: window.close(); break;
            default: break;
            }
        }
        }
        {
        sf::View view = window.getView();
        view.setCenter(float(window.getSize().x) / 2.f,
                       float(window.getSize().y) / 2.f);
        window.setView(view);
        test_map.apply_view(view);
        }
        window.clear();
        for (auto & layer : test_map) {
            window.draw(*layer);
        }
        for (auto & spt : sprites) {
            window.draw(spt);
        }
        window.display();
    }
}
