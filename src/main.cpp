#include "main.hpp"

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720

#define MAX_ITERATIONS  10000

sf::Vector2f screen_to_fractal(sf::Vector2i screen_pos, sf::RenderWindow* window)
{
    double area_height = 5.f * ((double)window->getSize().y / (double)window->getSize().x);
	double x = (((double)screen_pos.x / (double)window->getSize().x) - 0.5) * 5.f; // + position[0];
	double y = -((((double)screen_pos.y / (double)window->getSize().y) - 0.5) * area_height); // - position[1];

    return sf::Vector2f(x,y);
}

void initColorPallet() {
    //GLOBAL_PALLET.setBackgroundColor(sf::Glsl::Vec4(0.0, 0.0, 0.0, 1.0));
    GLOBAL_PALLET.registerPallet("original", {
            sf::Glsl::Vec4(  0.f / 255.f,   7.f / 255.f, 100.f / 255.f, 1.f),
            sf::Glsl::Vec4( 32.f / 255.f, 107.f / 255.f, 203.f / 255.f, 1.f),
            sf::Glsl::Vec4(237.f / 255.f, 255.f / 255.f, 255.f / 255.f, 1.f),
            sf::Glsl::Vec4(255.f / 255.f, 170.f / 255.f,   0.f / 255.f, 1.f),
            sf::Glsl::Vec4(  0.f / 255.f,   2.f / 255.f,   0.f / 255.f, 1.f),
            sf::Glsl::Vec4(  0.f / 255.f,   7.f / 255.f, 100.f / 255.f, 1.f)
    });

    GLOBAL_PALLET.registerPallet("RGB", {
        sf::Glsl::Vec4(255.f / 255.f,   0.f / 255.f,   0.f / 255.f, 1.f),
        sf::Glsl::Vec4(255.f / 255.f, 255.f / 255.f,   0.f / 255.f, 1.f),
        sf::Glsl::Vec4(  0.f / 255.f, 255.f / 255.f,   0.f / 255.f, 1.f),
        sf::Glsl::Vec4(  0.f / 255.f, 255.f / 255.f, 255.f / 255.f, 1.f),
        sf::Glsl::Vec4(  0.f / 255.f,   0.f / 255.f, 255.f / 255.f, 1.f),
        sf::Glsl::Vec4(255.f / 255.f,   0.f / 255.f, 255.f / 255.f, 1.f),
        sf::Glsl::Vec4(255.f / 255.f,   0.f / 255.f,   0.f / 255.f, 1.f)
    });

    GLOBAL_PALLET.registerPallet("Black And White", {
        sf::Glsl::Vec4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 1.f),
        sf::Glsl::Vec4(  0.f / 255.f,   0.f / 255.f,   0.f / 255.f, 1.f),
        sf::Glsl::Vec4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 1.f)
    });

    GLOBAL_PALLET.registerPallet("Fire", {
        sf::Glsl::Vec4( 20.f / 255.f,   0.f / 255.f,   0.f / 255.f, 1.f),
		sf::Glsl::Vec4(255.f / 255.f,  20.f / 255.f,   0.f / 255.f, 1.f),
		sf::Glsl::Vec4(255.f / 255.f, 200.f / 255.f,   0.f / 255.f, 1.f),
		sf::Glsl::Vec4(255.f / 255.f,  20.f / 255.f,   0.f / 255.f, 1.f),
		sf::Glsl::Vec4( 20.f / 255.f,   0.f / 255.f,   0.f / 255.f, 1.f)
    });
}

int main(int argc, char *argv[]) {

    //GLOBAL_PALLET = ColorPalet();    
    int CURRENT_COLOR_INDEX = 0;
    initColorPallet();

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Tutorial", sf::Style::Default);
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::Texture tex;
    tex.create(WINDOW_WIDTH, WINDOW_HEIGHT);
    sf::Sprite spr(tex);
    
    sf::Shader shader;
    shader.loadFromFile("../shaders/shader.frag", sf::Shader::Fragment);
    if (!shader.isAvailable()) {
        std::cout << "The shader is not available\n";
        return -1;
    }

    int iterations = 50;
    float scale = 1.5f;
    bool smooth = true;

    std::vector<sf::Glsl::Vec4> current_colors = GLOBAL_PALLET.getPallet(CURRENT_COLOR_INDEX);
    sf::Vector2f current_center = sf::Vector2f(0.f, 0.f);

    shader.setUniform("resolution", sf::Vector2f(window.getSize().x, window.getSize().y));
    shader.setUniform("iterations", iterations);
    shader.setUniformArray("pallet", palletToArray(current_colors), current_colors.size());
    shader.setUniform("scale", scale);
    shader.setUniform("smoth", smooth);
    shader.setUniform("colors_nb", (int) current_colors.size() -1);
    shader.setUniform("background_color", GLOBAL_PALLET.background_color);
    shader.setUniform("x_offset", current_center.x);
    shader.setUniform("y_offset", current_center.y);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                // ratio from window
                // define the ratio from the window 
                float ratio_x = (((float) window.getSize().x / (float) WINDOW_WIDTH) * scale) / 100;
                float ratio_y = (((float) window.getSize().y / (float) WINDOW_HEIGHT) * scale) / 100;


                if (event.key.code == sf::Keyboard::Right) {
                    current_center.x += ratio_x;
                    shader.setUniform("x_offset", current_center.x);
                } else if (event.key.code == sf::Keyboard::Left) {
                    current_center.x -= ratio_x;
                    shader.setUniform("x_offset", current_center.x);
                } else if (event.key.code == sf::Keyboard::Up) {
                    current_center.y -= ratio_y;
                    shader.setUniform("y_offset", current_center.y);
                } else if (event.key.code == sf::Keyboard::Down) {
                    current_center.y += ratio_y;
                    shader.setUniform("y_offset", current_center.y);
                }

                if (event.key.code == sf::Keyboard::Add) {
                    if (MAX_ITERATIONS > iterations) {
                        iterations += 1;
                        shader.setUniform("iterations", iterations);
                    }
                } else if (event.key.code == sf::Keyboard::Subtract) {
                    if (1 < iterations) {
                        iterations -= 1;
                        shader.setUniform("iterations", iterations);
                    }
                } else if (event.key.code == sf::Keyboard::A) {
                    scale *= 1.05f;
                    shader.setUniform("scale", scale);
                } else if (event.key.code == sf::Keyboard::Z) {
                    scale *= 0.95f;
                    shader.setUniform("scale", scale);
                }
            }
            if (event.type == sf::Event::Resized) {
                // update resolution?   
                shader.setUniform("resolution", sf::Vector2f(window.getSize().x, window.getSize().y));
            }
            /*
            if (event.type == sf::Event::MouseButtonPressed && event.key.code == sf::Mouse::Left) {
                sf::Vector2i mouse_pos_screen = sf::Mouse::getPosition(window);
                sf::Vector2f mouse_pos_fractal = screen_to_fractal(mouse_pos_screen, &window);               
                shader.setUniform("v_center", current_center);
                //std::cout << "v_center: " << mouse_pos_fractal.x << " " << mouse_pos_fractal.y << "\n";
            }*/
        }
        
        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();
        if (ImGui::SliderInt("Iterations", &iterations, 1, MAX_ITERATIONS)) {
            shader.setUniform("iterations", iterations);
        }
        ImGui::Separator();
        ImGui::Text("Colors Settings");

        const char *actual_string_color = GLOBAL_PALLET.pallet_names[CURRENT_COLOR_INDEX].c_str();
        if (ImGui::BeginCombo("Color Pallet", actual_string_color)) {
            for (size_t i = 0; i < GLOBAL_PALLET.pallet_names.size(); i++) {
                bool is_selected = CURRENT_COLOR_INDEX == (int) i;
                if (ImGui::Selectable(GLOBAL_PALLET.pallet_names[i].c_str(), &is_selected)) {
                    CURRENT_COLOR_INDEX = i;
                }
                if (is_selected) {
                    current_colors = GLOBAL_PALLET.getPallet(i);
                    
                    shader.setUniform("colors_nb", (int) current_colors.size() -1);
                    shader.setUniformArray("pallet", palletToArray(current_colors), current_colors.size());
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::ColorEdit3("Background Color", (float*) &GLOBAL_PALLET.background_color)) {
            shader.setUniform("background_color", GLOBAL_PALLET.background_color);
        }

        if (ImGui::Checkbox("Smooth", &smooth)) {
            shader.setUniform("smoth", smooth);
        }
        ImGui::End();

        window.clear();
        window.draw(spr, &shader);
        
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return EXIT_SUCCESS;
}