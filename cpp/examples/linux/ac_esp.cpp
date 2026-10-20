/*
 * pyMeow C++ example -- AssaultCube ESP (Linux)
 * Port of examples/linux/ac_esp.py (AssaultCube v1.3.0.2)
 *
 * Build (all sources in cpp/src plus raylib and raygui):
 *   g++ -std=c++17 -I../.. -o ac_esp ac_esp.cpp (sources of ../src)
 *       -lX11 -lXtst -lXinerama -lraylib -lm -lpthread -ldl -lrt -lGL
 */
#include <pymeow.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr bool kDebug = false;
constexpr bool kRapidFire = true;
constexpr bool kAmmoHack = true;

const char* const kProcessName = "linux_64_client";
const char* const kWindowTitle = "AssaultCube";

namespace Pointer {
constexpr pm::Address entityList = 0x1A3520;
constexpr pm::Address localPlayer = 0x1A3518;
constexpr pm::Address viewMatrix = 0x1B4FCC;
} /* namespace Pointer */

namespace Offsets {
constexpr pm::Address name = 0x219;
constexpr pm::Address health = 0x100;
constexpr pm::Address team = 0x320;
constexpr pm::Address pos = 0x8;
} /* namespace Offsets */

struct Colors {
    static pm::Color cyan() { return pm::getColor("cyan"); }
    static pm::Color orange() { return pm::getColor("orange"); }
    static pm::Color white() { return pm::getColor("white"); }
};

/* One player entity of the game. Reading a dead / invalid entity throws, the
 * caller catches it and skips the entity (like the Python example does). */
class Entity {
public:
    Entity(const pm::Process& process, pm::Address address)
        : m_address(address),
          m_name(pm::readString(process, address + Offsets::name)),
          m_health(pm::readInt(process, address + Offsets::health)),
          m_team(pm::readInt(process, address + Offsets::team)),
          m_pos3d(pm::readVec3(process, address + Offsets::pos)),
          m_color(m_team == 1 ? Colors::cyan() : Colors::orange()) {}

    void renderSnapline() const {
        pm::drawLine(static_cast<float>(pm::getScreenWidth()) / 2.0f, static_cast<float>(pm::getScreenHeight()), m_pos2d.x,
                     m_pos2d.y, m_color);
    }

    void renderInfo() const { pm::drawText(m_name, m_pos2d.x, m_pos2d.y, 15.0f, Colors::white()); }

    void setScreenPosition(pm::Vec2 pos) { m_pos2d = pos; }

    pm::Address address() const { return m_address; }
    int health() const { return m_health; }
    int team() const { return m_team; }
    const std::string& name() const { return m_name; }
    pm::Vec3 position() const { return m_pos3d; }

private:
    pm::Address m_address;
    std::string m_name;
    int m_health;
    int m_team;
    pm::Vec3 m_pos3d;
    pm::Color m_color;
    pm::Vec2 m_pos2d;
};

pm::Matrix toMatrix(const std::vector<float>& values) {
    pm::Matrix matrix{};
    for (std::size_t i = 0; i < values.size() && i < matrix.size(); ++i) matrix[i] = values[i];
    return matrix;
}

} /* namespace */

int main() {
    try {
        pm::Process process = pm::openProcess(kProcessName, kDebug);
        const pm::Address base = pm::getModule(process, kProcessName).base;
        /* AssaultCube stores its entity pointers in 32 bit slots. */
        const pm::Address entityList = static_cast<pm::Address>(pm::readInt(process, base + Pointer::entityList));

        if (kRapidFire) {
            const std::vector<pm::Address> rapidScan = pm::aobScanModule(process, kProcessName, "89 01 48 8B 43");
            if (!rapidScan.empty()) {
                pm::pageProtection(process, rapidScan[0], 7); /* PROT_READ|PROT_WRITE|PROT_EXEC */
                pm::writeBytes(process, rapidScan[0], {0x90, 0x90});
            }
        }

        if (kAmmoHack) {
            const std::vector<pm::Address> ammoScan = pm::aobScanModule(process, kProcessName, "83 00 FF 48 8B 43");
            if (!ammoScan.empty()) {
                pm::pageProtection(process, ammoScan[0], 7);
                pm::writeBytes(process, ammoScan[0], {0x90, 0x90, 0x90});
            }
        }

        pm::overlayInit(kWindowTitle, 144);
        while (pm::overlayLoop()) {
            const pm::Address localPlayer = static_cast<pm::Address>(pm::readInt(process, base + Pointer::localPlayer));
            const pm::Matrix matrix = toMatrix(pm::readFloats(process, base + Pointer::viewMatrix, 16));

            pm::beginDrawing();
            pm::drawFps(0.0f, 0.0f);

            for (int i = 0; i < 31; ++i) {
                const pm::Address entityAddress =
                    static_cast<pm::Address>(pm::readInt(process, entityList + static_cast<pm::Address>(i) * 8));
                if (entityAddress == 0 || entityAddress == localPlayer) continue;

                try {
                    Entity entity(process, entityAddress);
                    if (entity.health() <= 0) continue;

                    const pm::WtsResult screen = pm::worldToScreenNoExc(matrix, entity.position());
                    if (!screen.inBounds) continue;

                    entity.setScreenPosition(screen.pos);
                    entity.renderSnapline();
                    entity.renderInfo();
                } catch (const pm::Error&) {
                    continue; /* entity vanished or is not readable */
                }
            }
            pm::endDrawing();
        }
        pm::closeProcess(process);
        return 0;
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "pyMeow error: %s\n", e.what());
        return 1;
    }
}
