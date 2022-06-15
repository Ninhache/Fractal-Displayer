#include "main.hpp"

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720

#define MAX_ITERATIONS  1000



int main(int argc, char *argv[]) {

    //GLOBAL_PALLET = ColorPalet();    
    int CURRENT_COLOR_INDEX = 0;

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

    shader.setUniform("resolution", sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    shader.setUniform("iterations", iterations);
    shader.setUniformArray("pallet", palletToArray(current_colors), current_colors.size());
    shader.setUniform("scale", scale);
    shader.setUniform("smoth", smooth);
    shader.setUniform("colors_nb", (int) current_colors.size());

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Add) {
                    if (MAX_ITERATIONS > iterations) iterations += 1;
                } else if (event.key.code == sf::Keyboard::Subtract) {
                    if (1 < iterations) iterations -= 1;
                } else if (event.key.code == sf::Keyboard::A) {
                    scale *= 1.05f;
                } else if (event.key.code == sf::Keyboard::Z) {
                    scale *= 0.95f;
                }
            }
        }
        
        ImGui::SFML::Update(window, deltaClock.restart());

        //ImGui::ShowDemoWindow();
        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::SliderInt("Iterations", &iterations, 1, MAX_ITERATIONS);
        ImGui::Separator();
        ImGui::Text("Colors Settings");

        /*  
        if (ImGui::Button("Change Pallet")) {
            current_colors = GLOBAL_PALLET.getPallet(rand() % GLOBAL_PALLET.pallet_pairs.size());
        }*/

        //ImGui::ListBox("Pallet", 0, GLOBAL_PALLET.pallet_names.size());
        /*ImGui::BeginListBox("Pallet", &CURRENT_COLOR_INDEX);
        for (int i = 0; i < GLOBAL_PALLET.pallet_names.size(); i++) {
            if (ImGui::Selectable(GLOBAL_PALLET.pallet_names[i].c_str(), CURRENT_COLOR_INDEX == i)) {
                CURRENT_COLOR_INDEX = i;
            }
        }
        ImGui::EndListBox();
        */
        if (ImGui::BeginCombo("Multiselectable", "preview")) {
            for (int i = 0; i < GLOBAL_PALLET.pallet_names.size(); i++) {
                bool is_selected = CURRENT_COLOR_INDEX == i;
                if (ImGui::Selectable(GLOBAL_PALLET.pallet_names[i].c_str(), &is_selected)) {
                    CURRENT_COLOR_INDEX = i;
                }
                if (is_selected) {
                    current_colors = GLOBAL_PALLET.getPallet(i);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Smooth", &smooth);
        ImGui::End();

        shader.setUniform("iterations", iterations);
        shader.setUniform("scale", scale);
        shader.setUniform("smoth", smooth);
        shader.setUniform("colors_nb", (int) current_colors.size() -1);
        shader.setUniformArray("pallet", palletToArray(current_colors), current_colors.size());

        window.clear();
        window.draw(spr, &shader);
        
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}