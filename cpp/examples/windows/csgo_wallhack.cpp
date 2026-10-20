/*
 * pyMeow C++ example -- CS:GO wallhack (Windows)
 * Port of examples/windows/csgo_wallhack.py
 * Credits to https://github.com/danielkrupinski/OneByteWallhack
 *
 * Build (MSVC, memory core only -- no raylib needed):
 *   cl /EHsc /std:c++17 /DPYMEOW_NO_GRAPHICS /I..\.. csgo_wallhack.cpp (sources of ..\src)
 *      psapi.lib user32.lib
 */
#include <pymeow.hpp>

#include <cstdio>

int main() {
    try {
        pm::Process process = pm::openProcess("csgo.exe");
        const std::vector<pm::Address> scan = pm::aobScanModule(process, "client.dll", "33 C0 83 FA ?? B9 20");
        if (scan.empty()) {
            std::fprintf(stderr, "pattern not found\n");
            return 1;
        }

        const pm::Address address = scan[0] + 4;
        pm::writeByte(process, address, pm::readByte(process, address) == 1 ? 2 : 1);
        pm::closeProcess(process);
        return 0;
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "pyMeow error: %s\n", e.what());
        return 1;
    }
}
