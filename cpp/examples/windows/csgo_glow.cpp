/*
 * pyMeow C++ example -- CS:GO glow (Windows)
 * Port of examples/windows/csgo_glow.py
 *
 * The Python example downloads the offsets from hazedumper at runtime
 * (https://github.com/frk1/hazedumper). The C++ example keeps them as constants:
 * update them for your CS:GO version, or fetch them yourself and fill the
 * Offsets struct before calling run().
 *
 * Build (MSVC, all sources in cpp/src):
 *   cl /EHsc /std:c++17 /DPYMEOW_NO_GRAPHICS /I..\.. csgo_glow.cpp (sources of ..\src)
 *      psapi.lib user32.lib
 */
#include <pymeow.hpp>

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

/* hazedumper "signatures" + "netvars" (update for your game version). */
struct Offsets {
    pm::Address dwLocalPlayer = 0xDB15DC;
    pm::Address dwEntityList = 0x4D426EC;
    pm::Address dwGlowObjectManager = 0x525C7F8;
    pm::Address m_iGlowIndex = 0xA428;
    pm::Address m_iTeamNum = 0xF4;
};

/* Layout of the glow object of CS:GO (ctypes GlowStruct of the Python example).
 * #pragma pack(1) keeps the field offsets identical. */
#pragma pack(push, 1)
struct GlowStruct {
    std::uint8_t pad0[8];
    float r;
    float g;
    float b;
    float a;
    std::uint8_t pad1[16];
    bool renderWhenOccluded;
    bool renderWhenUnoccluded;
};
#pragma pack(pop)

} /* namespace */

int main() {
    try {
        const Offsets offsets;
        const pm::Process csgo = pm::openProcess("csgo.exe");
        const pm::Address client = pm::getModule(csgo, "client.dll").base;

        const pm::Color cyan = pm::getColor("cyan");
        const pm::Color orange = pm::getColor("orange");

        for (;;) {
            pm::Address localPlayer = 0;
            try {
                localPlayer = static_cast<pm::Address>(pm::readInt(csgo, client + offsets.dwLocalPlayer));
            } catch (const pm::MemoryError&) {
                continue;
            }
            if (localPlayer == 0) continue;

            /* every 4th entry of the entity list holds an entity address */
            const std::vector<std::int32_t> entityBuffer =
                pm::readInts(csgo, client + offsets.dwEntityList, 128);

            for (std::size_t i = 0; i < entityBuffer.size(); i += 4) {
                const pm::Address entity = static_cast<pm::Address>(static_cast<std::uint32_t>(entityBuffer[i]));
                if (entity == 0 || entity == localPlayer) continue;

                try {
                    const pm::Address glowManager = pm::readUInt(csgo, client + offsets.dwGlowObjectManager);
                    const int glowIndex = pm::readInt(csgo, entity + offsets.m_iGlowIndex);
                    const pm::Address glowAddress = glowManager + static_cast<pm::Address>(glowIndex) * 0x38;

                    const int team = pm::readInt(csgo, entity + offsets.m_iTeamNum);
                    const pm::Color color = (team != 2) ? cyan : orange;

                    /* pm::read<T> / pm::write<T> replace r_ctype / w_ctype */
                    GlowStruct glow = pm::read<GlowStruct>(csgo, glowAddress);
                    glow.r = static_cast<float>(color.r);
                    glow.g = static_cast<float>(color.g);
                    glow.b = static_cast<float>(color.b);
                    glow.a = static_cast<float>(color.a);
                    glow.renderWhenOccluded = true;
                    glow.renderWhenUnoccluded = false;
                    pm::write<GlowStruct>(csgo, glowAddress, glow);
                } catch (const pm::MemoryError&) {
                    continue;
                }
            }
        }
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "pyMeow error: %s\n", e.what());
        return 1;
    }
}
