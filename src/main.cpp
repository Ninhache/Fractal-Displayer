#include "main.hpp"

#include <algorithm>
#include <cmath>

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720

#define MAX_ITERATIONS  10000

// While the user is actively navigating we render at 1/LOW_RES_DIVISOR resolution
// for responsiveness, then snap to full resolution once interaction stops.
static const unsigned LOW_RES_DIVISOR = 4;
// Seconds of inactivity before the full-resolution render kicks in.
static const float SETTLE_DELAY = 0.25f;

// IDLE = nothing changed, blit the cached full-res image (no shader cost).
// INTERACTING = something is changing, render cheap low-res every frame.
// SETTLING = interaction just stopped, render full-res once then go IDLE.
enum class RenderState { IDLE, INTERACTING, SETTLING };

// Modern dark ImGui theme with a red accent.
static void setupImGuiTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.f;
    style.FrameRounding  = 4.f;
    style.GrabRounding   = 4.f;
    style.TabRounding    = 4.f;
    style.WindowPadding  = ImVec2(12.f, 12.f);
    style.FramePadding   = ImVec2(8.f, 4.f);
    style.ItemSpacing    = ImVec2(8.f, 8.f);

    ImVec4* colors = style.Colors;
    const ImVec4 accent       = ImVec4(0.48f, 0.16f, 0.18f, 1.00f);
    const ImVec4 accentHover  = ImVec4(0.90f, 0.22f, 0.24f, 1.00f);
    const ImVec4 accentActive = ImVec4(0.70f, 0.12f, 0.14f, 1.00f);

    colors[ImGuiCol_Text]             = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_WindowBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBg]          = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.78f, 0.16f, 0.18f, 0.45f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.78f, 0.16f, 0.18f, 0.65f);
    colors[ImGuiCol_TitleBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = accent;
    colors[ImGuiCol_CheckMark]        = accentHover;
    colors[ImGuiCol_SliderGrab]       = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_Button]           = accent;
    colors[ImGuiCol_ButtonHovered]    = accentHover;
    colors[ImGuiCol_ButtonActive]     = accentActive;
    colors[ImGuiCol_Header]           = accent;
    colors[ImGuiCol_HeaderHovered]    = accentHover;
    colors[ImGuiCol_HeaderActive]     = accentActive;
    colors[ImGuiCol_Separator]        = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
}

// (Re)create the full- and low-resolution render targets for a given window size.
static void resizeRenderTargets(sf::RenderTexture& full, sf::RenderTexture& low,
                                unsigned w, unsigned h) {
    full.create(w, h);
    low.create(std::max(1u, w / LOW_RES_DIVISOR), std::max(1u, h / LOW_RES_DIVISOR));
}

// Run the fractal shader once over the whole render texture.
static void renderFractal(sf::RenderTexture& rt, sf::Shader& shader) {
    sf::Vector2u sz = rt.getSize();
    shader.setUniform("resolution", sf::Vector2f((float) sz.x, (float) sz.y));
    sf::RectangleShape quad(sf::Vector2f((float) sz.x, (float) sz.y));
    rt.clear();
    rt.draw(quad, &shader);
    rt.display();
}

// Below ~this scale the plain-double shader path runs out of precision (~10^13 zoom),
// so we switch the shader to the heavy double-double path.
static const double DD_THRESHOLD = 1e-12;

// Minimal double-double (df64) for the CPU view center. At deep zoom a plain double
// (~15 digits) can't even represent where we are; df64 carries ~32. Same two-sum as
// the shader so both sides agree. Only the ops incremental navigation needs.
struct dd { double hi, lo; };
static dd dd_quick_two_sum(double a, double b) { double s = a + b; double e = b - (s - a); return {s, e}; }
static dd dd_two_sum(double a, double b) { double s = a + b, bb = s - a; double e = (a - (s - bb)) + (b - bb); return {s, e}; }
static dd dd_add_d(dd a, double b) { dd s = dd_two_sum(a.hi, b); s.lo += a.lo; return dd_quick_two_sum(s.hi, s.lo); }

// Express a df64 as a 5-float expansion (descending magnitude). Two floats per double
// would lose 4 bits and leave a hi/lo gap; this residual chain keeps the full ~104 bits
// so they survive SFML's float-only uniforms and reconstruct exactly on the GPU.
static void dd_to_float5(dd v, float out[5]) {
    dd rem = v;
    for (int k = 0; k < 5; ++k) {
        float f = (float) rem.hi;
        out[k] = f;
        rem = dd_add_d(rem, -(double) f);
    }
}

// SFML can't upload double uniforms, so split a double into a hi/lo float pair the
// shader recombines.
static void setDoubleAs2f(sf::Shader& shader, const std::string& a, const std::string& b, double v) {
    float hi = (float) v;
    float lo = (float) (v - (double) hi);
    shader.setUniform(a, hi);
    shader.setUniform(b, lo);
}

// Point the display sprite at the given render texture and stretch it to fill the window.
static void updateSprite(sf::Sprite& spr, const sf::RenderTexture& rt,
                         sf::Vector2u windowSize) {
    spr.setTexture(rt.getTexture(), true);
    spr.setScale((float) windowSize.x / (float) rt.getSize().x,
                 (float) windowSize.y / (float) rt.getSize().y);
    spr.setPosition(0.f, 0.f);
}

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
    setupImGuiTheme();

    sf::RenderTexture fractalRT_full;
    sf::RenderTexture fractalRT_low;
    sf::Sprite        fractalSprite;
    resizeRenderTargets(fractalRT_full, fractalRT_low,
                        window.getSize().x, window.getSize().y);

    sf::Shader shader;
    shader.loadFromFile("../shaders/shader.frag", sf::Shader::Fragment);
    if (!shader.isAvailable()) {
        std::cout << "The shader is not available\n";
        return -1;
    }

    int iterations = 50;
    // Geometry kept in double (center as double-double) so deep-zoom navigation doesn't
    // lose precision before the values even reach the shader. See pushView for the upload.
    double scale = 1.5;
    bool smooth = true;

    std::vector<sf::Glsl::Vec4> current_colors = GLOBAL_PALLET.getPallet(CURRENT_COLOR_INDEX);
    dd center_x = {0.0, 0.0};
    dd center_y = {0.0, 0.0};

    // Push the view geometry to the shader: scale as a single double, the offsets as
    // double-double (each double split into two floats), plus the df64-mode flag that
    // engages the heavy path only when zoomed past the plain-double precision wall.
    auto pushView = [&]() {
        setDoubleAs2f(shader, "scale_hi", "scale_lo", scale);
        float xoff[5], yoff[5];
        dd_to_float5(center_x, xoff);
        dd_to_float5(center_y, yoff);
        shader.setUniformArray("x_off", xoff, 5);
        shader.setUniformArray("y_off", yoff, 5);
        shader.setUniform("dd_mode", scale < DD_THRESHOLD);
    };

    shader.setUniform("resolution", sf::Vector2f(window.getSize().x, window.getSize().y));
    shader.setUniform("iterations", iterations);
    shader.setUniformArray("pallet", palletToArray(current_colors), current_colors.size());
    shader.setUniform("smoth", smooth);
    shader.setUniform("colors_nb", (int) current_colors.size() -1);
    shader.setUniform("background_color", GLOBAL_PALLET.background_color);
    pushView();

    // --- Render state machine: only recompute the fractal when something changes ---
    RenderState renderState = RenderState::INTERACTING; // force a render on the first frame
    sf::Clock   settleTimer;

    // Navigation tuning constants
    const float PAN_SPEED  = 1.0f;  // complex-plane units per second at scale = 1
    const float ZOOM_RATE  = 2.0f;  // held Z/A: zoom factor per second
    const float WHEEL_RATE = 1.5f;  // mouse wheel: zoom factor per notch

    // Flag the view as changed so the next frames re-render (and the settle timer restarts).
    auto markDirty = [&]() {
        renderState = RenderState::INTERACTING;
        settleTimer.restart();
    };

    sf::Clock deltaClock;
    sf::Clock frameClock;
    while (window.isOpen()) {
        // Cap dt so a hitch (or breakpoint) can't teleport the view.
        float dt = std::min(frameClock.restart().asSeconds(), 0.1f);
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                // Iterations stay event-driven — they are a discrete control and the
                // OS key-repeat is a natural throttle. Pan/zoom are polled below instead.
                if (event.key.code == sf::Keyboard::Add) {
                    if (MAX_ITERATIONS > iterations) {
                        iterations += 1;
                        shader.setUniform("iterations", iterations);
                        markDirty();
                    }
                } else if (event.key.code == sf::Keyboard::Subtract) {
                    if (1 < iterations) {
                        iterations -= 1;
                        shader.setUniform("iterations", iterations);
                        markDirty();
                    }
                }
            }
            if (event.type == sf::Event::Resized) {
                // Keep SFML's view in sync with the framebuffer, otherwise the sprite
                // no longer fills the resized window, and resize the render targets.
                window.setView(sf::View(sf::FloatRect(0.f, 0.f,
                                                       (float) event.size.width,
                                                       (float) event.size.height)));
                resizeRenderTargets(fractalRT_full, fractalRT_low,
                                    event.size.width, event.size.height);
                markDirty();
            }
            if (event.type == sf::Event::MouseWheelScrolled &&
                !ImGui::GetIO().WantCaptureMouse) {
                // Zoom toward the cursor: keep the complex point under the pointer fixed.
                double f  = (event.mouseWheelScroll.delta > 0) ? (1.0 / WHEEL_RATE) : WHEEL_RATE;
                double ds = scale * f - scale;

                double winW = (double) window.getSize().x;
                double winH = (double) window.getSize().y;
                double mx   = (double) event.mouseWheelScroll.x;
                double my   = (double) event.mouseWheelScroll.y;

                center_x = dd_add_d(center_x, ((2.0 * mx - winW) / winH) * ds);
                center_y = dd_add_d(center_y, ((winH - 2.0 * my) / winH) * ds);
                scale *= f;

                pushView();
                markDirty();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // --- Polled, frame-rate-independent pan & zoom (ignored while ImGui has focus) ---
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            bool moved = false;
            double panStep = (double) PAN_SPEED * scale * (double) dt; // scales with zoom

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { center_x = dd_add_d(center_x, -panStep); moved = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { center_x = dd_add_d(center_x,  panStep); moved = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { center_y = dd_add_d(center_y, -panStep); moved = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { center_y = dd_add_d(center_y,  panStep); moved = true; }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { scale *= std::pow(ZOOM_RATE, (double) dt);       moved = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) { scale *= std::pow(1.0 / ZOOM_RATE, (double) dt); moved = true; }

            if (moved) {
                pushView();
                markDirty();
            }
        }

        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();
        if (ImGui::SliderInt("Iterations", &iterations, 1, MAX_ITERATIONS)) {
            shader.setUniform("iterations", iterations);
            markDirty();
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
                    markDirty();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::ColorEdit3("Background Color", (float*) &GLOBAL_PALLET.background_color)) {
            shader.setUniform("background_color", GLOBAL_PALLET.background_color);
            markDirty();
        }

        if (ImGui::Checkbox("Smooth", &smooth)) {
            shader.setUniform("smoth", smooth);
            markDirty();
        }
        ImGui::End();

        // --- Adaptive render: low-res while interacting, full-res once settled, cached when idle ---
        if (renderState == RenderState::INTERACTING) {
            renderFractal(fractalRT_low, shader);
            updateSprite(fractalSprite, fractalRT_low, window.getSize());
            if (settleTimer.getElapsedTime().asSeconds() > SETTLE_DELAY) {
                renderState = RenderState::SETTLING;
            }
        } else if (renderState == RenderState::SETTLING) {
            renderFractal(fractalRT_full, shader);
            updateSprite(fractalSprite, fractalRT_full, window.getSize());
            renderState = RenderState::IDLE;
        }
        // IDLE: fractalSprite already holds the cached full-res image — nothing to recompute.

        window.clear();
        window.draw(fractalSprite);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return EXIT_SUCCESS;
}
