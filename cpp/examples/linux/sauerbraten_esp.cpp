/*
 * pyMeow C++ example -- Sauerbraten ESP (Linux)
 * Port of examples/linux/sauerbraten_esp.py (Cube 2: Sauerbraten)
 *
 * Build (all sources in cpp/src plus raylib and raygui):
 *   g++ -std=c++17 -I../.. -o sauerbraten_esp sauerbraten_esp.cpp (sources of ../src)
 *       -lX11 -lXtst -lXinerama -lraylib -lm -lpthread -ldl -lrt -lGL
 */
#include <pymeow.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace {

const char* const kProcessName = "linux_64_client";
const char* const kWindowTitle = "Cube 2: Sauerbraten";

namespace Offsets {
constexpr pm::Address entityList = 0x5EDB70;
constexpr pm::Address local = 0x53A600;
constexpr pm::Address playerCount = 0x5EDB7C;
constexpr pm::Address viewMatrix = 0x5D06E0;
constexpr pm::Address gameMode = 0x52940C;

constexpr pm::Address health = 0x178;
constexpr pm::Address armor = 0x180;
constexpr pm::Address state = 0x77;
constexpr pm::Address name = 0x274;
constexpr pm::Address team = 0x378;
constexpr pm::Address viewAngles = 0x3C;
} /* namespace Offsets */

struct Colors {
    static pm::Color white() { return pm::getColor("white"); }
    static pm::Color cyan() { return pm::getColor("cyan"); }
    static pm::Color orange() { return pm::getColor("orange"); }
};

class Entity {
public:
    /* Throws when the entity is not alive, exactly like the Python example. */
    Entity(const pm::Process& process, pm::Address address) : m_address(address) {
        m_alive = pm::readByte(process, address + Offsets::state) == 0;
        if (!m_alive) throw pm::Error("Entity not alive");

        m_health = pm::readInt(process, address + Offsets::health);
        m_armor = pm::readInt(process, address + Offsets::armor);
        m_headPos3d = pm::readVec3(process, address);
        m_feetPos3d = pm::Vec3(m_headPos3d.x, m_headPos3d.y, m_headPos3d.z - 15.0f);
        m_name = pm::readString(process, address + Offsets::name);
        m_team = pm::readString(process, address + Offsets::team);
        m_viewAngles = pm::readVec2(process, address + Offsets::viewAngles);
    }

    bool alive() const { return m_alive; }
    int health() const { return m_health; }
    const std::string& team() const { return m_team; }
    pm::Vec3 headPosition() const { return m_headPos3d; }

    void project(const pm::Matrix& matrix) {
        const pm::WtsResult head = pm::worldToScreenNoExc(matrix, m_headPos3d);
        const pm::WtsResult feet = pm::worldToScreenNoExc(matrix, m_feetPos3d);
        if (!head.inBounds || !feet.inBounds) throw pm::OutOfBoundsError();
        m_headPos2d = head.pos;
        m_feetPos2d = feet.pos;
    }

    void setColor(pm::Color color) { m_color = color; }
    void setDistance(int distance) { m_distance = distance; }

    void drawBox() const {
        const float head = m_feetPos2d.y - m_headPos2d.y;
        const float width = head / 2.0f;
        const float center = width / 2.0f;
        const float posX = m_headPos2d.x - center;
        const float posY = m_headPos2d.y - center / 2.0f;
        const float height = head + center / 2.0f;

        pm::drawRectangle(posX, posY, width, height, pm::fadeColor(m_color, 0.2f));
        pm::drawRectangleLines(posX, posY, width, height, m_color);
    }

    void drawInfo() const {
        const std::string text = m_name + " (" + std::to_string(m_distance) + "m)";
        const float size = static_cast<float>(pm::measureText(text, 10)) / 2.0f;
        pm::drawText(text, m_feetPos2d.x - size, m_feetPos2d.y + 10.0f, 10.0f, Colors::white());
    }

private:
    pm::Address m_address;
    bool m_alive = false;
    int m_health = 0;
    int m_armor = 0;
    int m_distance = 0;
    std::string m_name;
    std::string m_team;
    pm::Vec2 m_viewAngles;
    pm::Vec3 m_headPos3d;
    pm::Vec3 m_feetPos3d;
    pm::Vec2 m_headPos2d;
    pm::Vec2 m_feetPos2d;
    pm::Color m_color = Colors::white();
};

pm::Matrix toMatrix(const std::vector<float>& values) {
    pm::Matrix matrix{};
    for (std::size_t i = 0; i < values.size() && i < matrix.size(); ++i) matrix[i] = values[i];
    return matrix;
}

const pm::Process* gProcess = nullptr;
pm::Address gBase = 0;

/* Collects every visible entity of the current frame. */
std::vector<Entity> entityLoop() {
    std::vector<Entity> entities;

    const int playerCount = pm::readInt(*gProcess, gBase + Offsets::playerCount);
    if (playerCount <= 1) return entities;

    const pm::Address listAddress = static_cast<pm::Address>(pm::readInt64(*gProcess, gBase + Offsets::entityList));
    const std::vector<std::int64_t> entityBuffer = pm::readInts64(*gProcess, listAddress, static_cast<std::size_t>(playerCount));
    if (entityBuffer.empty()) return entities;

    Entity local(*gProcess, static_cast<pm::Address>(entityBuffer[0])); /* throws when invalid */
    const pm::Matrix matrix = toMatrix(pm::readFloats(*gProcess, gBase + Offsets::viewMatrix, 16));

    for (std::size_t i = 1; i < entityBuffer.size(); ++i) {
        try {
            Entity entity(*gProcess, static_cast<pm::Address>(entityBuffer[i]));
            if (!entity.alive()) continue;

            entity.project(matrix);
            entity.setDistance(static_cast<int>(pm::vec3Distance(local.headPosition(), entity.headPosition()) / 3.0f));
            entity.setColor(entity.team() == local.team() ? Colors::cyan() : Colors::orange());
            entities.push_back(entity);
        } catch (const pm::Error&) {
            continue;
        }
    }
    return entities;
}

} /* namespace */

int main() {
    try {
        pm::Process process = pm::openProcess(kProcessName);
        gProcess = &process;
        gBase = pm::getModule(process, kProcessName).base;
        pm::overlayInit(kWindowTitle, 200);
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    while (pm::overlayLoop()) {
        pm::beginDrawing();
        pm::drawFps(10.0f, 10.0f);
        try {
            for (const Entity& entity : entityLoop()) {
                entity.drawBox();
                entity.drawInfo();
            }
        } catch (const pm::Error&) {
            /* the local player is gone for this frame */
        }
        pm::endDrawing();
    }

    pm::closeProcess(*const_cast<pm::Process*>(gProcess));
    return 0;
}
