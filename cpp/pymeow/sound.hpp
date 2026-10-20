/*
 * pyMeow C++ -- sound (port of sound.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Sounds are referenced by an int id, like in the Python module:
 *
 *   pm::sound::init();
 *   int beep = pm::sound::load("beep.wav");
 *   pm::sound::play(beep);
 *
 * Requires raylib (audio device). Not compiled when PYMEOW_NO_GRAPHICS is set.
 */
#ifndef PYMEOW_SOUND_HPP
#define PYMEOW_SOUND_HPP

#include "common.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#include <string>

namespace pymeow {
namespace sound {

PYMEOW_API void init();
PYMEOW_API void deinit();
PYMEOW_API bool isReady();

/* Loads a sound file and returns its id (throws Error when the id is unknown
 * in the calls below). */
PYMEOW_API int load(const std::string& fileName);
PYMEOW_API void unload(int soundId);
PYMEOW_API bool exists(int soundId);

PYMEOW_API void play(int soundId);
PYMEOW_API void pause(int soundId);
PYMEOW_API void resume(int soundId);
PYMEOW_API void stop(int soundId);
PYMEOW_API void playMulti(int soundId);
PYMEOW_API void stopMulti();
/* Volume in percent (0 - 100), like the Python set_sound_volume(). */
PYMEOW_API void setVolume(int soundId, int volume);
PYMEOW_API bool isPlaying(int soundId);

} /* namespace sound */
} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */

#endif /* PYMEOW_SOUND_HPP */
