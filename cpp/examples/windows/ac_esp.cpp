/*
 * pyMeow C++ example -- AssaultCube ESP (Windows)
 * Port of examples/windows/ac_esp.py
 *
 * Build (MSVC, all sources in cpp/src plus raylib and raygui):
 *   cl /EHsc /std:c++17 /I..\.. ac_esp.cpp (sources of ..\src)
 *      raylib.lib opengl32.lib gdi32.lib user32.lib psapi.lib
 */
#include <pymeow.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace {

const char* const kProcessName = "ac_client.exe";
const char* const kWindowTitle = "AssaultCube";

namespace Pointer {
constexpr pm::Address playerCount = 0x18AC0C;
constexpr pm::Address entityList = 0x18AC04;
constexpr pm::Address localPlayer = 0x18AC00;
constexpr pm::Address viewMatrix = 0x17DFD0;
} /* namespace Pointer */

namespace Offsets {
constexpr pm::Address name = 0x205;
constexpr pm::Address health = 0xEC;
constexpr pm::Address armor = 0xF0;
constexpr pm::Address team = 0x30C;
constexpr pm::Address pos = 0x4;
constexpr pm::Address fpos = 0x28;
} /* namespace Offsets */

struct Colors {
    static pm::Color cyan() { return pm::getColor("cyan"); }
    static pm::Color orange() { return pm::getColor("orange"); }
    static pm::Color white() { return pm::getColor("white"); }
    static pm::Color black() { return pm::getColor("black"); }
};

pm::Matrix toMatrix(const std::vector<float>& values) {
    pm::Matrix matrix{};
    for (std::size_t i = 0; i < values.size() && i < matrix.size(); ++i) matrix[i] = values[i];
    return matrix;
}

class Entity {
public:
    Entity(const pm::Process& process, pm::Address address)
        : m_address(address),
          m_health(pm::readInt(process, address + Offsets::health)),
          m_armor(pm::readInt(process, address + Offsets::armor)),
          m_team(pm::readInt(process, address + Offsets::team)),
          m_name(pm::readString(process, address + Offsets::name)),
          m_pos3d(pm::readVec3(process, address + Offsets::pos)),
          m_fpos3d(pm::readVec3(process, address + Offsets::fpos)),
          m_color(m_team != 1 ? Colors::cyan() : Colors::orange()) {}

    /* Projects head and feet, returns false when either is behind the camera. */
    bool worldToScreen(const pm::Matrix& matrix) {
        const pm::WtsResult head = pm::worldToScreenNoExc(matrix, m_pos3d);
        const pm::WtsResult feet = pm::worldToScreenNoExc(matrix, m_fpos3d);
        if (!head.inBounds || !feet.inBounds) return false;
        m_pos2d = head.pos;
        m_fpos2d = feet.pos;
        m_headHeight = m_fpos2d.y - m_pos2d.y;
        m_width = m_headHeight / 2.0f;
        m_center = m_width / 2.0f;
        return true;
    }

    void drawBox() const {
        const float posX = m_pos2d.x - m_center;
        const float posY = m_pos2d.y - m_center / 2.0f;
        const float height = m_headHeight + m_center / 2.0f;
        pm::drawRectangle(posX, posY, m_width, height, pm::fadeColor(m_color, 0.3f));
        pm::drawRectangleLines(posX, posY, m_width, height, m_color, 1.2f);
    }

    void drawName() const {
        const float textSize = static_cast<float>(pm::measureText(m_name, 15)) / 2.0f;
        pm::drawText(m_name, m_pos2d.x - textSize, m_pos2d.y, 15.0f, Colors::white());
    }

    void drawHealth() const {
        const float centerX = m_pos2d.x - m_center;
        const float centerY = m_pos2d.y - m_center / 2.0f;
        pm::drawCircleSector(centerX, centerY, m_center / 3.0f + 2.0f, 0.0f, 360.0f, 0, Colors::black());
        pm::drawCircleSector(centerX, centerY, m_center / 3.0f, 0.0f, 360.0f / 100.0f * static_cast<float>(m_health), 0,
                             m_color);
    }

private:
    pm::Address m_address;
    int m_health = 0;
    int m_armor = 0;
    int m_team = 0;
    std::string m_name;
    pm::Vec3 m_pos3d;
    pm::Vec3 m_fpos3d;
    pm::Vec2 m_pos2d;
    pm::Vec2 m_fpos2d;
    pm::Color m_color;
    float m_headHeight = 0.0f;
    float m_width = 0.0f;
    float m_center = 0.0f;
};

} /* namespace */

int main() {
    try {
        const pm::Process process = pm::openProcess(kProcessName);
        const pm::Address base = pm::getModule(process, kProcessName).base;

        pm::OverlayConfig config;
        config.target = kWindowTitle;
        config.fps = 144;
        config.trackTarget = true;
        pm::overlayInit(config);

        while (pm::overlayLoop()) {
            pm::beginDrawing();
            pm::drawFps(10.0f, 10.0f);

            const int playerCount = pm::readInt(process, base + Pointer::playerCount);
            if (playerCount > 1) {
                const pm::Address listAddress = static_cast<pm::Address>(pm::readInt(process, base + Pointer::entityList));
                const std::vector<std::int32_t> entityBuffer =
                    pm::readInts(process, listAddress, static_cast<std::size_t>(playerCount));
                const pm::Matrix matrix = toMatrix(pm::readFloats(process, base + Pointer::viewMatrix, 16));

                for (std::size_t i = 1; i < entityBuffer.size(); ++i) {
                    try {
                        Entity entity(process, static_cast<pm::Address>(static_cast<std::uint32_t>(entityBuffer[i])));
                        if (!entity.worldToScreen(matrix)) continue;
                        entity.drawBox();
                        entity.drawName();
                        entity.drawHealth();
                    } catch (const pm::Error&) {
                        continue;
                    }
                }
            }
            pm::endDrawing();
        }
        return 0;
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "pyMeow error: %s\n", e.what());
        return 1;
    }
}
