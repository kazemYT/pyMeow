/*
 * pyMeow C++ example -- memory basics (no graphics, no raylib needed)
 *
 * Build (Linux):
 *   g++ -std=c++17 -DPYMEOW_NO_GRAPHICS -I.. -o memory_basics memory_basics.cpp
 *       (plus every source in ../src) -lX11 -lXtst -lXinerama
 *
 * Build (Windows):
 *   cl /EHsc /std:c++17 /DPYMEOW_NO_GRAPHICS /I..\.. memory_basics.cpp ..\..\src\*.cpp
 *
 * The example opens its own process, so it always has a valid target.
 */
#include <pymeow.hpp>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace {

int currentPid() {
#if defined(_WIN32)
    return static_cast<int>(::GetCurrentProcessId());
#else
    return static_cast<int>(::getpid());
#endif
}

} /* namespace */

int main() {
    std::printf("pyMeow C++ %s on %s\n\n", pm::version(), pm::systemName());

    try {
        /* ---------------------------------------------------- 1. processes */
        std::printf("--- processes\n");
        const std::vector<pm::Process> processes = pm::enumProcesses();
        std::printf("running processes: %zu\n", processes.size());
        for (std::size_t i = 0; i < processes.size() && i < 5; ++i) {
            std::printf("  [%d] %s\n", processes[i].pid, processes[i].name.c_str());
        }

        const int pid = currentPid();
        std::printf("pid %d exists: %s\n", pid, pm::pidExists(pid) ? "yes" : "no");
        std::printf("pid %d name: %s\n", pid, pm::getProcessName(pid).c_str());

        /* ------------------------------------------------------- 2. open it */
        std::printf("\n--- open process\n");
        pm::Process process = pm::openProcess(pid, /*debug = */ false);
        std::printf("opened %s (pid %d), base 0x%llx, 64 bit: %s\n", process.name.c_str(), process.pid,
                    static_cast<unsigned long long>(process.base), pm::is64Bit(process) ? "yes" : "no");

        /* ------------------------------------------------------ 3. modules */
        std::printf("\n--- modules\n");
        const std::vector<pm::Module> modules = pm::enumModules(process);
        for (std::size_t i = 0; i < modules.size() && i < 5; ++i) {
            std::printf("  %-24s base 0x%012llx size %llu\n", modules[i].name.c_str(),
                        static_cast<unsigned long long>(modules[i].base),
                        static_cast<unsigned long long>(modules[i].size));
        }
        const pm::Module mainModule = pm::getModule(process, modules.front().name);
        std::printf("main module: %s\n", mainModule.name.c_str());

        const std::vector<pm::MemoryRegion> regions = pm::enumMemoryRegions(process, mainModule);
        std::printf("memory regions of %s: %zu\n", mainModule.name.c_str(), regions.size());
        for (std::size_t i = 0; i < regions.size() && i < 3; ++i) {
            std::printf("  0x%012llx size %-8llu %s\n", static_cast<unsigned long long>(regions[i].start),
                        static_cast<unsigned long long>(regions[i].size), regions[i].perms.c_str());
        }

        /* ------------------------------------------------- 4. read & write */
        std::printf("\n--- read / write\n");
        std::int32_t health = 100;
        float speed = 1.5f;
        pm::Vec3 position(1.0f, 2.0f, 3.0f);

        std::printf("health   : %d\n", pm::readInt(process, reinterpret_cast<pm::Address>(&health)));
        pm::writeInt(process, reinterpret_cast<pm::Address>(&health), 1337);
        std::printf("health   : %d (after writeInt)\n", health);

        std::printf("speed    : %f\n", static_cast<double>(pm::readFloat(process, reinterpret_cast<pm::Address>(&speed))));
        pm::write<float>(process, reinterpret_cast<pm::Address>(&speed), 9.75f); /* the template form */
        std::printf("speed    : %f (after write<float>)\n", static_cast<double>(speed));

        const pm::Vec3 readPosition = pm::readVec3(process, reinterpret_cast<pm::Address>(&position));
        std::printf("position : (%f, %f, %f)\n", static_cast<double>(readPosition.x), static_cast<double>(readPosition.y),
                    static_cast<double>(readPosition.z));

        std::int32_t numbers[4] = {1, 2, 3, 4};
        const std::vector<std::int32_t> all = pm::readInts(process, reinterpret_cast<pm::Address>(numbers), 4);
        std::printf("ints     : %d %d %d %d\n", all[0], all[1], all[2], all[3]);
        pm::writeBytes(process, reinterpret_cast<pm::Address>(numbers), {9, 9, 9, 9});
        std::printf("bytes    : %d %d %d %d (after writeBytes)\n", numbers[0], numbers[1], numbers[2], numbers[3]);

        char name[32] = "pyMeow";
        std::printf("string   : %s\n", pm::readString(process, reinterpret_cast<pm::Address>(name), 30).c_str());

        /* ------------------------------------------------ 5. pointer chains */
        std::printf("\n--- pointer chain\n");
        std::uint64_t value = 0xABCD;
        std::uint64_t* level2 = &value;
        std::uint64_t** level1 = &level2;
        const pm::Address resolved =
            pm::pointerChain64(process, reinterpret_cast<pm::Address>(&level1), {0, 0});
        std::printf("resolved : 0x%llx -> %llu\n", static_cast<unsigned long long>(resolved),
                    static_cast<unsigned long long>(pm::readUInt64(process, resolved)));

        /* ------------------------------------------------------- 6. AOB scan */
        std::printf("\n--- aob scan\n");
        std::vector<std::uint8_t> haystack(1024, 0x00);
        haystack[512] = 0x89;
        haystack[513] = 0x01;
        haystack[514] = 0x48;
        haystack[515] = 0x8B;
        haystack[516] = 0x43;
        const pm::Address haystackAddress = reinterpret_cast<pm::Address>(haystack.data());

        const std::vector<pm::Address> hits =
            pm::aobScanRange(process, "89 01 48 8B 43", haystackAddress, haystackAddress + haystack.size());
        for (pm::Address hit : hits) std::printf("  found at 0x%llx (offset %llu)\n", static_cast<unsigned long long>(hit),
                                                 static_cast<unsigned long long>(hit - haystackAddress));

        const std::vector<pm::Address> wild =
            pm::aobScanRange(process, "89 ?? 48 8B", haystackAddress, haystackAddress + haystack.size(), true);
        std::printf("  relative wildcard hit: %llu\n", wild.empty() ? 0ull : static_cast<unsigned long long>(wild[0]));

        /* ------------------------------------------------------ 7. errors */
        std::printf("\n--- error handling\n");
        try {
            pm::getModule(process, "no_such_module.dll");
        } catch (const pm::NotFoundError& e) {
            std::printf("  NotFoundError: %s\n", e.what());
        }
        std::int32_t dummy = 0;
        if (!pm::tryReadPointer(process, static_cast<pm::Address>(0x10), &dummy, sizeof(dummy))) {
            std::printf("  reading address 0x10 failed as expected: %s\n", pm::errorString().c_str());
        }

        pm::closeProcess(process);
        std::printf("\ndone\n");
        return 0;
    } catch (const pm::Error& e) {
        std::printf("pyMeow error: %s\n", e.what());
        return 1;
    }
}
