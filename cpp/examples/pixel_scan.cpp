/*
 * pyMeow C++ example -- pixel color scanner
 * Port of examples/pixel_scan.py
 *
 * Searches a 150x150 pixel area around the mouse cursor for green and red
 * pixels and draws random lines between the matches inside the overlay.
 *
 * Build (all sources in cpp/src plus raylib and raygui):
 *   g++ -std=c++17 -I.. -o pixel_scan pixel_scan.cpp (sources of ../src)
 *       -lX11 -lXtst -lXinerama -lraylib -lm -lpthread -ldl -lrt -lGL
 */
#include <pymeow.hpp>

#include <cstdlib>
#include <vector>

namespace {

constexpr int kSearchArea = 150;
constexpr int kMarkDensity = 50;
constexpr float kSimilarity = 92.0f;

} /* namespace */

int main() {
    const std::pair<int, int> resolution = pm::getDisplayResolution();
    const std::vector<pm::Color> searchColors = {pm::getColor("green"), pm::getColor("red")};
    const pm::Color markColor = pm::getColor("gold");

    pm::overlayInit();

    while (pm::overlayLoop()) {
        pm::sleepMs(1);
        const pm::Vec2 mouse = pm::mousePosition();
        const float x = mouse.x - kSearchArea / 2.0f;
        const float y = mouse.y - kSearchArea / 2.0f;

        const bool outOfBounds = mouse.x + kSearchArea > resolution.first || mouse.y + kSearchArea > resolution.second ||
                                 mouse.x - kSearchArea < 0 || mouse.y - kSearchArea < 0;
        if (outOfBounds) continue;

        /* coordinates are relative to the captured region */
        const std::vector<pm::Pixel> matches =
            pm::pixelSearchColors(static_cast<int>(x), static_cast<int>(y), kSearchArea, kSearchArea, searchColors, kSimilarity);

        pm::beginDrawing();
        pm::drawFps(10.0f, 10.0f);
        for (int i = 0; i < kMarkDensity && !matches.empty(); ++i) {
            const pm::Pixel& p1 = matches[static_cast<std::size_t>(std::rand()) % matches.size()];
            const pm::Pixel& p2 = matches[static_cast<std::size_t>(std::rand()) % matches.size()];
            pm::drawLine(static_cast<float>(p1.x) + x, static_cast<float>(p1.y) + y, static_cast<float>(p2.x) + x,
                         static_cast<float>(p2.y) + y, markColor);
        }
        pm::endDrawing();
    }
    return 0;
}
