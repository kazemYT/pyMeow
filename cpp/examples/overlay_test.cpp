/*
 * pyMeow C++ example -- overlay / GUI playground
 * Port of examples/overlay_test.py
 *
 * Build (all sources in cpp/src plus raylib and raygui):
 *   g++ -std=c++17 -I.. -o overlay_test overlay_test.cpp (sources of ../src)
 *       -lX11 -lXtst -lXinerama -lraylib -lm -lpthread -ldl -lrt -lGL
 */
#include <pymeow.hpp>

#include <cstdlib>
#include <string>
#include <vector>

namespace {

int randomInt(int low, int high) { return low + (std::rand() % (high - low + 1)); }

} /* namespace */

int main() {
    pm::overlayInit();

    int fps = pm::getMonitorRefreshRate();
    pm::setFps(fps);
    pm::toggleMouse();   /* the GUI needs the mouse, the overlay is passthrough by default */
    pm::gui::setFade(0.9f);

    int radius = 50;
    const int width = pm::getScreenWidth();
    const int height = pm::getScreenHeight();
    int x = width / 2;
    int y = height / 2;
    int speed = 5;
    const int maxSpeed = 100;
    bool ballLeft = false;
    bool ballDown = false;
    pm::Color circleColor = pm::getColor("#000000");

    std::vector<pm::Vec2> stars;
    stars.reserve(300);
    for (int i = 0; i < 300; ++i) {
        stars.push_back(pm::vec2(static_cast<float>(randomInt(0, width)), static_cast<float>(randomInt(0, height))));
    }

    while (pm::overlayLoop()) {
        /* bouncing ball */
        if (ballLeft) {
            if (x > width - radius) ballLeft = false;
            else x += speed;
        } else {
            if (x < -1 + radius) ballLeft = true;
            else x -= speed;
        }
        if (ballDown) {
            if (y > height - radius) ballDown = false;
            else y += speed;
        } else {
            if (y < -1 + radius) ballDown = true;
            else y -= speed;
        }

        pm::beginDrawing();
        for (const pm::Vec2& star : stars) pm::drawPixel(star.x, star.y, pm::getColor("#ffffff"));
        pm::drawCircle(static_cast<float>(x), static_cast<float>(y), static_cast<float>(radius), circleColor);
        pm::drawFps(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f);

        /* GUI: every widget keeps its own state, identified by its position */
        if (pm::gui::button(0.0f, 0.0f, 100.0f, 50.0f, "Increase speed")) {
            if (speed != maxSpeed) speed += 5;
        } else if (pm::gui::button(0.0f, 50.0f, 100.0f, 50.0f, "Decrease speed")) {
            if (speed != 0) speed -= 5;
        } else if (pm::gui::button(100.0f, 0.0f, 100.0f, 50.0f, "Change Color")) {
            circleColor = pm::newColor(static_cast<std::uint8_t>(randomInt(0, 255)),
                                       static_cast<std::uint8_t>(randomInt(0, 255)),
                                       static_cast<std::uint8_t>(randomInt(0, 255)),
                                       static_cast<std::uint8_t>(randomInt(0, 255)));
        } else if (pm::gui::button(100.0f, 50.0f, 100.0f, 50.0f, "Mouse passthrough")) {
            pm::toggleMouse();
        } else if (pm::gui::button(200.0f, 0.0f, 100.0f, 50.0f, "Increase FPS")) {
            ++fps;
            pm::setFps(fps);
        } else if (pm::gui::button(200.0f, 50.0f, 100.0f, 50.0f, "Decrease FPS")) {
            --fps;
            pm::setFps(fps);
        }

        pm::gui::progressBar(350.0f, 10.0f, 200.0f, 30.0f, "Speed: ", std::to_string(speed), static_cast<float>(speed), 0.0f,
                             static_cast<float>(maxSpeed));
        radius = static_cast<int>(pm::gui::slider(350.0f, 60.0f, 200.0f, 30.0f, "Radius", std::to_string(radius),
                                                 static_cast<float>(radius), 0.0f, 150.0f));
        pm::drawText("Exit with 'END'", 50.0f, 100.0f, 25.0f, pm::getColor("#ffffff"));
        pm::endDrawing();
    }
    return 0;
}
