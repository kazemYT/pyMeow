/*
 * pyMeow C++ -- sound implementation (port of sound.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/sound.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#  include "pymeow/error.hpp"

#  include <unordered_map>
#  include <vector>

namespace pymeow {
namespace sound {

namespace {

std::unordered_map<int, ::Sound> gSounds;
std::vector<::Sound> gAliases; /* playing aliases created by playMulti() */
int gNextSoundId = -1;

const ::Sound& requireSound(int soundId) {
    const auto it = gSounds.find(soundId);
    if (it == gSounds.end()) throw NotFoundError("Unknown Sound ID: " + std::to_string(soundId));
    return it->second;
}

} /* namespace */

void init() { ::InitAudioDevice(); }

void deinit() {
    stopMulti();
    gSounds.clear();
    ::CloseAudioDevice();
}

bool isReady() { return ::IsAudioDeviceReady(); }

int load(const std::string& fileName) {
    const ::Sound sound = ::LoadSound(fileName.c_str());
    if (sound.frameCount == 0) throw Error("loadSound failed: " + fileName);
    ++gNextSoundId;
    gSounds[gNextSoundId] = sound;
    return gNextSoundId;
}

void unload(int soundId) {
    const auto it = gSounds.find(soundId);
    if (it == gSounds.end()) return;
    ::UnloadSound(it->second);
    gSounds.erase(it);
}

bool exists(int soundId) { return gSounds.find(soundId) != gSounds.end(); }

void play(int soundId) { ::PlaySound(requireSound(soundId)); }
void pause(int soundId) { ::PauseSound(requireSound(soundId)); }
void resume(int soundId) { ::ResumeSound(requireSound(soundId)); }
void stop(int soundId) { ::StopSound(requireSound(soundId)); }

/* raylib 4+ removed PlaySoundMulti() / StopSoundMulti(): sound aliases that
 * share the sample data are used to play the same sound several times at once. */
void playMulti(int soundId) {
    const ::Sound source = requireSound(soundId);

    /* Recycle the aliases that already finished. */
    for (std::size_t i = 0; i < gAliases.size();) {
        if (::IsSoundPlaying(gAliases[i]) == false) {
            ::UnloadSoundAlias(gAliases[i]);
            gAliases.erase(gAliases.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    gAliases.push_back(::LoadSoundAlias(source));
    ::PlaySound(gAliases.back());
}

void stopMulti() {
    for (::Sound& alias : gAliases) {
        ::StopSound(alias);
        ::UnloadSoundAlias(alias);
    }
    gAliases.clear();
}

void setVolume(int soundId, int volume) { ::SetSoundVolume(requireSound(soundId), static_cast<float>(volume) / 100.0f); }

bool isPlaying(int soundId) { return ::IsSoundPlaying(requireSound(soundId)); }

} /* namespace sound */
} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */
