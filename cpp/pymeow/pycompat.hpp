/*
 * pyMeow C++ -- Python compatible names (optional)
 * https://github.com/qb-0/pyMeow
 *
 * The C++ port uses camelCase (pm::readInt, pm::gui::button, pm::sound::load).
 * This optional header adds the snake_case names of the Python module on top of
 * them, so an existing pyMeow script can be translated almost literally:
 *
 *   #include <pymeow/pycompat.hpp>
 *
 *   pm::Process proc = pm::open_process("ac_client.exe");
 *   pm::Address base = pm::get_module(proc, "ac_client.exe").base;
 *   int health       = pm::r_int(proc, base + 0xEC);
 *   pm::w_bytes(proc, address, {0x90, 0x90});
 *   pm::overlay_init("AssaultCube", 144);
 *   while (pm::overlay_loop()) { ... }
 *
 * Every alias is a thin forwarding wrapper: same behaviour, same exceptions,
 * all overloads and default arguments of the C++ API stay available.
 */
#ifndef PYMEOW_PYCOMPAT_HPP
#define PYMEOW_PYCOMPAT_HPP

#include "../pymeow.hpp"

#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/* Perfect forwarding keeps every overload (and the default arguments of the
 * real function) working, and works for void returns as well. */
#define PYMEOW_PY_ALIAS(py_name, cpp_target)                                                                       \
    template <typename... PyArgs>                                                                                  \
    inline auto py_name(PyArgs&&... args) -> decltype(cpp_target(std::forward<PyArgs>(args)...)) {                 \
        return cpp_target(std::forward<PyArgs>(args)...);                                                          \
    }

/* Braced init lists ({0x90, 0x90}) can not be deduced by a forwarding template,
 * so the array writers and the color search get an explicit overload as well. */
#define PYMEOW_PY_ALIAS_LIST(py_name, cpp_target, type)                                                                inline void py_name(const Process& process, Address address, std::initializer_list<type> data) {                       cpp_target(process, address, data);                                                                            }

namespace pymeow {

/* ============================================================ memory (core) */
PYMEOW_PY_ALIAS(enum_processes, enumProcesses)
PYMEOW_PY_ALIAS(pid_exists, pidExists)
PYMEOW_PY_ALIAS(process_exists, processExists)
PYMEOW_PY_ALIAS(process_running, processRunning)
PYMEOW_PY_ALIAS(get_process_id, getProcessId)
PYMEOW_PY_ALIAS(get_process_name, getProcessName)
PYMEOW_PY_ALIAS(get_process_path, getProcessPath)
PYMEOW_PY_ALIAS(open_process, openProcess)
PYMEOW_PY_ALIAS(close_process, closeProcess)
PYMEOW_PY_ALIAS(enum_modules, enumModules)
PYMEOW_PY_ALIAS(module_exists, moduleExists)
PYMEOW_PY_ALIAS(get_module, getModule)
PYMEOW_PY_ALIAS(find_module, findModule)
PYMEOW_PY_ALIAS(enum_memory_regions, enumMemoryRegions)
PYMEOW_PY_ALIAS(is_64_bit, is64Bit)
PYMEOW_PY_ALIAS(get_os_error, getOsError)
PYMEOW_PY_ALIAS(error_string, errorString)
PYMEOW_PY_ALIAS(system_name, systemName)
PYMEOW_PY_ALIAS(page_protection, pageProtection)
PYMEOW_PY_ALIAS(allocate_memory, allocateMemory)
PYMEOW_PY_ALIAS(free_memory, freeMemory)
PYMEOW_PY_ALIAS(get_proc_address, getProcAddress)
PYMEOW_PY_ALIAS(create_remote_thread, createRemoteThread)
PYMEOW_PY_ALIAS(inject_library, injectLibrary)
PYMEOW_PY_ALIAS(inject_shellcode, injectShellcode)
PYMEOW_PY_ALIAS(aob_scan_module, aobScanModule)
PYMEOW_PY_ALIAS(aob_scan_range, aobScanRange)
PYMEOW_PY_ALIAS(aob_scan_region, aobScanRegion)
PYMEOW_PY_ALIAS(aob_scan_bytes, aobScanBytes)
PYMEOW_PY_ALIAS(pointer_chain_32, pointerChain32)
PYMEOW_PY_ALIAS(pointer_chain_64, pointerChain64)
PYMEOW_PY_ALIAS(set_require_root, setRequireRoot)

/* ============================================================ memory (read) */
PYMEOW_PY_ALIAS(r_string, readString)
PYMEOW_PY_ALIAS(r_wstring, readWString)
PYMEOW_PY_ALIAS(r_int, readInt)
PYMEOW_PY_ALIAS(r_ints, readInts)
PYMEOW_PY_ALIAS(r_int8, readInt8)
PYMEOW_PY_ALIAS(r_ints8, readInts8)
PYMEOW_PY_ALIAS(r_int16, readInt16)
PYMEOW_PY_ALIAS(r_ints16, readInts16)
PYMEOW_PY_ALIAS(r_int64, readInt64)
PYMEOW_PY_ALIAS(r_ints64, readInts64)
PYMEOW_PY_ALIAS(r_uint, readUInt)
PYMEOW_PY_ALIAS(r_uints, readUInts)
PYMEOW_PY_ALIAS(r_uint16, readUInt16)
PYMEOW_PY_ALIAS(r_uints16, readUInts16)
PYMEOW_PY_ALIAS(r_uint64, readUInt64)
PYMEOW_PY_ALIAS(r_uints64, readUInts64)
PYMEOW_PY_ALIAS(r_byte, readByte)
PYMEOW_PY_ALIAS(r_bytes, readBytes)
PYMEOW_PY_ALIAS(r_float, readFloat)
PYMEOW_PY_ALIAS(r_floats, readFloats)
PYMEOW_PY_ALIAS(r_float64, readFloat64)
PYMEOW_PY_ALIAS(r_floats64, readFloats64)
PYMEOW_PY_ALIAS(r_bool, readBool)
PYMEOW_PY_ALIAS(r_vec2, readVec2)
PYMEOW_PY_ALIAS(r_vec3, readVec3)

/* pm.r_ctype(process, address, GlowStruct()) -> pm::r_ctype<GlowStruct>(process, address)
 * (the instance overload is kept so a literal translation still compiles) */
template <class T>
inline T r_ctype(const Process& process, Address address) {
    return read<T>(process, address);
}
template <class T>
inline T r_ctype(const Process& process, Address address, const T&) {
    return read<T>(process, address);
}

/* pm.r(process, address, "int") / pm.r(process, address, "int", 32) */
inline Value r(const Process& process, Address address, const std::string& type) {
    return readValue(process, address, dataTypeFromString(type));
}
inline std::vector<Value> r(const Process& process, Address address, const std::string& type, std::size_t count) {
    return readValues(process, address, dataTypeFromString(type), count);
}

/* =========================================================== memory (write) */
PYMEOW_PY_ALIAS(w_string, writeString)
PYMEOW_PY_ALIAS(w_string_z, writeStringZ)
PYMEOW_PY_ALIAS(w_wstring, writeWString)
PYMEOW_PY_ALIAS(w_int, writeInt)
PYMEOW_PY_ALIAS(w_ints, writeInts)
PYMEOW_PY_ALIAS(w_int8, writeInt8)
PYMEOW_PY_ALIAS(w_ints8, writeInts8)
PYMEOW_PY_ALIAS(w_int16, writeInt16)
PYMEOW_PY_ALIAS(w_ints16, writeInts16)
PYMEOW_PY_ALIAS(w_int64, writeInt64)
PYMEOW_PY_ALIAS(w_ints64, writeInts64)
PYMEOW_PY_ALIAS(w_uint, writeUInt)
PYMEOW_PY_ALIAS(w_uints, writeUInts)
PYMEOW_PY_ALIAS(w_uint16, writeUInt16)
PYMEOW_PY_ALIAS(w_uints16, writeUInts16)
PYMEOW_PY_ALIAS(w_uint64, writeUInt64)
PYMEOW_PY_ALIAS(w_uints64, writeUInts64)
PYMEOW_PY_ALIAS(w_byte, writeByte)
PYMEOW_PY_ALIAS(w_bytes, writeBytes)
PYMEOW_PY_ALIAS(w_float, writeFloat)
PYMEOW_PY_ALIAS(w_floats, writeFloats)
PYMEOW_PY_ALIAS(w_float64, writeFloat64)
PYMEOW_PY_ALIAS(w_floats64, writeFloats64)
PYMEOW_PY_ALIAS(w_bool, writeBool)
PYMEOW_PY_ALIAS(w_vec2, writeVec2)
PYMEOW_PY_ALIAS(w_vec3, writeVec3)

PYMEOW_PY_ALIAS_LIST(w_ints, writeInts, std::int32_t)
PYMEOW_PY_ALIAS_LIST(w_ints8, writeInts8, std::int8_t)
PYMEOW_PY_ALIAS_LIST(w_ints16, writeInts16, std::int16_t)
PYMEOW_PY_ALIAS_LIST(w_ints64, writeInts64, std::int64_t)
PYMEOW_PY_ALIAS_LIST(w_uints, writeUInts, std::uint32_t)
PYMEOW_PY_ALIAS_LIST(w_uints16, writeUInts16, std::uint16_t)
PYMEOW_PY_ALIAS_LIST(w_uints64, writeUInts64, std::uint64_t)
PYMEOW_PY_ALIAS_LIST(w_floats, writeFloats, float)
PYMEOW_PY_ALIAS_LIST(w_floats64, writeFloats64, double)
PYMEOW_PY_ALIAS_LIST(w_bytes, writeBytes, std::uint8_t)

template <class T>
inline void w_ctype(const Process& process, Address address, const T& data) {
    write<T>(process, address, data);
}

/* pm.w(process, address, data[, type]) -- the type is taken from the value
 * itself, the string form is accepted for a literal translation. */
inline void w(const Process& process, Address address, const Value& data) { writeValue(process, address, data); }
inline void w(const Process& process, Address address, const Value& data, const std::string&) {
    writeValue(process, address, data);
}
inline void w(const Process& process, Address address, const std::vector<Value>& data, const std::string& type) {
    writeValues(process, address, dataTypeFromString(type), data);
}

#if defined(PYMEOW_WITH_RAYLIB)

/* ================================================================== overlay */
PYMEOW_PY_ALIAS(overlay_init, overlayInit)
PYMEOW_PY_ALIAS(overlay_loop, overlayLoop)
PYMEOW_PY_ALIAS(overlay_close, overlayClose)
PYMEOW_PY_ALIAS(overlay_config, overlayConfig)
PYMEOW_PY_ALIAS(overlay_ready, overlayReady)
PYMEOW_PY_ALIAS(toggle_mouse, toggleMouse)
PYMEOW_PY_ALIAS(begin_drawing, beginDrawing)
PYMEOW_PY_ALIAS(end_drawing, endDrawing)
PYMEOW_PY_ALIAS(get_fps, getFps)
PYMEOW_PY_ALIAS(set_fps, setFps)
PYMEOW_PY_ALIAS(get_screen_width, getScreenWidth)
PYMEOW_PY_ALIAS(get_screen_height, getScreenHeight)
PYMEOW_PY_ALIAS(get_window_position, getWindowPosition)
PYMEOW_PY_ALIAS(set_window_position, setWindowPosition)
PYMEOW_PY_ALIAS(set_window_size, setWindowSize)
PYMEOW_PY_ALIAS(set_window_title, setWindowTitle)
PYMEOW_PY_ALIAS(set_window_icon, setWindowIcon)
PYMEOW_PY_ALIAS(set_window_monitor, setWindowMonitor)
PYMEOW_PY_ALIAS(set_window_flag, setWindowFlag)
PYMEOW_PY_ALIAS(get_window_handle, getWindowHandle)
PYMEOW_PY_ALIAS(set_log_level, setLogLevel)
PYMEOW_PY_ALIAS(take_screenshot, takeScreenshot)
PYMEOW_PY_ALIAS(get_monitor_count, getMonitorCount)
PYMEOW_PY_ALIAS(get_monitor_name, getMonitorName)
PYMEOW_PY_ALIAS(get_monitor_refresh_rate, getMonitorRefreshRate)

/* =================================================================== render */
PYMEOW_PY_ALIAS(draw_fps, drawFps)
PYMEOW_PY_ALIAS(draw_text, drawText)
PYMEOW_PY_ALIAS(draw_pixel, drawPixel)
PYMEOW_PY_ALIAS(draw_line, drawLine)
PYMEOW_PY_ALIAS(draw_circle, drawCircle)
PYMEOW_PY_ALIAS(draw_circle_lines, drawCircleLines)
PYMEOW_PY_ALIAS(draw_circle_sector, drawCircleSector)
PYMEOW_PY_ALIAS(draw_circle_sector_lines, drawCircleSectorLines)
PYMEOW_PY_ALIAS(draw_ring, drawRing)
PYMEOW_PY_ALIAS(draw_ring_lines, drawRingLines)
PYMEOW_PY_ALIAS(draw_ellipse, drawEllipse)
PYMEOW_PY_ALIAS(draw_ellipse_lines, drawEllipseLines)
PYMEOW_PY_ALIAS(draw_rectangle, drawRectangle)
PYMEOW_PY_ALIAS(draw_rectangle_lines, drawRectangleLines)
PYMEOW_PY_ALIAS(draw_rectangle_rounded, drawRectangleRounded)
PYMEOW_PY_ALIAS(draw_rectangle_rounded_lines, drawRectangleRoundedLines)
PYMEOW_PY_ALIAS(draw_triangle, drawTriangle)
PYMEOW_PY_ALIAS(draw_triangle_lines, drawTriangleLines)
PYMEOW_PY_ALIAS(draw_poly, drawPoly)
PYMEOW_PY_ALIAS(draw_poly_lines, drawPolyLines)
PYMEOW_PY_ALIAS(load_texture, loadTexture)
PYMEOW_PY_ALIAS(load_texture_bytes, loadTextureBytes)
PYMEOW_PY_ALIAS(draw_texture, drawTexture)
PYMEOW_PY_ALIAS(unload_texture, unloadTexture)
PYMEOW_PY_ALIAS(load_font, loadFont)
PYMEOW_PY_ALIAS(unload_font, unloadFont)
PYMEOW_PY_ALIAS(font_exists, fontExists)
PYMEOW_PY_ALIAS(draw_font, drawFont)
PYMEOW_PY_ALIAS(measure_font, measureFont)
PYMEOW_PY_ALIAS(measure_text, measureText)
PYMEOW_PY_ALIAS(check_collision_point_rec, checkCollisionPointRec)
PYMEOW_PY_ALIAS(check_collision_recs, checkCollisionRecs)
PYMEOW_PY_ALIAS(check_collision_circle_rec, checkCollisionCircleRec)
PYMEOW_PY_ALIAS(check_collision_circles, checkCollisionCircles)
PYMEOW_PY_ALIAS(check_collision_lines, checkCollisionLines)
PYMEOW_PY_ALIAS(world_to_screen, worldToScreen)
PYMEOW_PY_ALIAS(world_to_screen_noexc, worldToScreenNoExc)

#endif /* PYMEOW_WITH_RAYLIB */

#if defined(PYMEOW_WITH_GUI)

/* ====================================================================== gui */
PYMEOW_PY_ALIAS(gui_window_box, gui::windowBox)
PYMEOW_PY_ALIAS(gui_group_box, gui::groupBox)
PYMEOW_PY_ALIAS(gui_line, gui::line)
PYMEOW_PY_ALIAS(gui_panel, gui::panel)
PYMEOW_PY_ALIAS(gui_scroll_panel, gui::scrollPanel)
PYMEOW_PY_ALIAS(gui_scroll_bar, gui::scrollBar)
PYMEOW_PY_ALIAS(gui_label, gui::label)
PYMEOW_PY_ALIAS(gui_button, gui::button)
PYMEOW_PY_ALIAS(gui_label_button, gui::labelButton)
PYMEOW_PY_ALIAS(gui_toggle, gui::toggle)
PYMEOW_PY_ALIAS(gui_check_box, gui::checkBox)
PYMEOW_PY_ALIAS(gui_combo_box, gui::comboBox)
PYMEOW_PY_ALIAS(gui_dropdown_box, gui::dropdownBox)
PYMEOW_PY_ALIAS(gui_text_box, gui::textBox)
PYMEOW_PY_ALIAS(gui_spinner, gui::spinner)
PYMEOW_PY_ALIAS(gui_slider, gui::slider)
PYMEOW_PY_ALIAS(gui_slider_bar, gui::sliderBar)
PYMEOW_PY_ALIAS(gui_progress_bar, gui::progressBar)
PYMEOW_PY_ALIAS(gui_status_bar, gui::statusBar)
PYMEOW_PY_ALIAS(gui_message_box, gui::messageBox)
PYMEOW_PY_ALIAS(gui_color_picker, gui::colorPicker)
PYMEOW_PY_ALIAS(gui_color_bar_alpha, gui::colorBarAlpha)
PYMEOW_PY_ALIAS(gui_color_bar_hue, gui::colorBarHue)
PYMEOW_PY_ALIAS(gui_load_style, gui::loadStyle)
PYMEOW_PY_ALIAS(gui_fade, gui::setFade)
PYMEOW_PY_ALIAS(gui_set_state, gui::setState)
PYMEOW_PY_ALIAS(gui_get_state, gui::getState)
PYMEOW_PY_ALIAS(gui_lock, gui::lock)
PYMEOW_PY_ALIAS(gui_unlock, gui::unlock)
PYMEOW_PY_ALIAS(gui_set_style, gui::setStyle)
PYMEOW_PY_ALIAS(gui_get_style, gui::getStyle)
PYMEOW_PY_ALIAS(gui_clear_widgets, gui::clearWidgets)

#endif /* PYMEOW_WITH_GUI */

/* ==================================================================== pixel */
PYMEOW_PY_ALIAS(pixel_enum_region, pixelEnumRegion)
PYMEOW_PY_ALIAS(pixel_enum_screen, pixelEnumScreen)
PYMEOW_PY_ALIAS(pixel_at_mouse, pixelAtMouse)
PYMEOW_PY_ALIAS(get_pixel, getPixel)
PYMEOW_PY_ALIAS(pixel_save_to_file, pixelSaveToFile)
PYMEOW_PY_ALIAS(save_pixels_as_png, savePixelsAsPng)
PYMEOW_PY_ALIAS(pixel_search_colors, pixelSearchColors)
PYMEOW_PY_ALIAS(for_each_pixel_in_region, forEachPixelInRegion)

inline std::vector<Pixel> pixel_search_colors(int x, int y, int width, int height, std::initializer_list<Color> colors,
                                             float similarity, bool absoluteCoords = false) {
    return pixelSearchColors(x, y, width, height, colors, similarity, absoluteCoords);
}

/* ==================================================================== input */
PYMEOW_PY_ALIAS(key_pressed, keyPressed)
PYMEOW_PY_ALIAS(press_key, pressKey)
PYMEOW_PY_ALIAS(release_key, releaseKey)
PYMEOW_PY_ALIAS(mouse_pressed, mousePressed)
PYMEOW_PY_ALIAS(mouse_move, mouseMove)
PYMEOW_PY_ALIAS(mouse_down, mouseDown)
PYMEOW_PY_ALIAS(mouse_up, mouseUp)
PYMEOW_PY_ALIAS(mouse_click, mouseClick)
PYMEOW_PY_ALIAS(mouse_position, mousePosition)

/* ==================================================================== utils */
PYMEOW_PY_ALIAS(new_color, newColor)
PYMEOW_PY_ALIAS(new_color_hex, newColorHex)
PYMEOW_PY_ALIAS(new_color_float, newColorFloat)
PYMEOW_PY_ALIAS(get_color, getColor)
PYMEOW_PY_ALIAS(parse_color, parseColor)
PYMEOW_PY_ALIAS(try_parse_color, tryParseColor)
PYMEOW_PY_ALIAS(all_colors, allColors)
PYMEOW_PY_ALIAS(is_color_name, isColorName)
PYMEOW_PY_ALIAS(fade_color, fadeColor)
PYMEOW_PY_ALIAS(compare_color_pct, compareColorPct)
PYMEOW_PY_ALIAS(get_display_resolution, getDisplayResolution)
PYMEOW_PY_ALIAS(get_window_info, getWindowInfo)
PYMEOW_PY_ALIAS(get_window_title, getWindowTitle)
PYMEOW_PY_ALIAS(run_time, runTime)
PYMEOW_PY_ALIAS(sleep_ms, sleepMs)

#if defined(PYMEOW_WITH_RAYLIB)

/* ==================================================================== sound */
PYMEOW_PY_ALIAS(sound_init, sound::init)
PYMEOW_PY_ALIAS(sound_deinit, sound::deinit)
PYMEOW_PY_ALIAS(sound_ready, sound::isReady)
PYMEOW_PY_ALIAS(sound_exists, sound::exists)
PYMEOW_PY_ALIAS(load_sound, sound::load)
PYMEOW_PY_ALIAS(unload_sound, sound::unload)
PYMEOW_PY_ALIAS(play_sound, sound::play)
PYMEOW_PY_ALIAS(pause_sound, sound::pause)
PYMEOW_PY_ALIAS(resume_sound, sound::resume)
PYMEOW_PY_ALIAS(stop_sound, sound::stop)
PYMEOW_PY_ALIAS(play_multisound, sound::playMulti)
PYMEOW_PY_ALIAS(stop_multisound, sound::stopMulti)
PYMEOW_PY_ALIAS(set_sound_volume, sound::setVolume)
PYMEOW_PY_ALIAS(is_sound_playing, sound::isPlaying)

#endif /* PYMEOW_WITH_RAYLIB */

/* ================================================================== vectors */
/* vec2() / vec3() keep their name, only the operations need an alias. */
PYMEOW_PY_ALIAS(vec2_add, vec2Add)
PYMEOW_PY_ALIAS(vec2_add_value, vec2AddValue)
PYMEOW_PY_ALIAS(vec2_subtract, vec2Subtract)
PYMEOW_PY_ALIAS(vec2_subtract_value, vec2SubtractValue)
PYMEOW_PY_ALIAS(vec2_multiply, vec2Multiply)
PYMEOW_PY_ALIAS(vec2_multiply_value, vec2MultiplyValue)
PYMEOW_PY_ALIAS(vec2_divide, vec2Divide)
PYMEOW_PY_ALIAS(vec2_divide_value, vec2DivideValue)
PYMEOW_PY_ALIAS(vec2_length, vec2Length)
PYMEOW_PY_ALIAS(vec2_length_sqr, vec2LengthSqr)
PYMEOW_PY_ALIAS(vec2_distance, vec2Distance)
PYMEOW_PY_ALIAS(vec2_distance_sqr, vec2DistanceSqr)
PYMEOW_PY_ALIAS(vec2_dot, vec2Dot)
PYMEOW_PY_ALIAS(vec2_normalize, vec2Normalize)
PYMEOW_PY_ALIAS(vec2_lerp, vec2Lerp)
PYMEOW_PY_ALIAS(vec2_closest, vec2Closest)
PYMEOW_PY_ALIAS(vec3_add, vec3Add)
PYMEOW_PY_ALIAS(vec3_add_value, vec3AddValue)
PYMEOW_PY_ALIAS(vec3_subtract, vec3Subtract)
PYMEOW_PY_ALIAS(vec3_subtract_value, vec3SubtractValue)
PYMEOW_PY_ALIAS(vec3_multiply, vec3Multiply)
PYMEOW_PY_ALIAS(vec3_multiply_value, vec3MultiplyValue)
PYMEOW_PY_ALIAS(vec3_divide, vec3Divide)
PYMEOW_PY_ALIAS(vec3_divide_value, vec3DivideValue)
PYMEOW_PY_ALIAS(vec3_length, vec3Length)
PYMEOW_PY_ALIAS(vec3_length_sqr, vec3LengthSqr)
PYMEOW_PY_ALIAS(vec3_distance, vec3Distance)
PYMEOW_PY_ALIAS(vec3_distance_sqr, vec3DistanceSqr)
PYMEOW_PY_ALIAS(vec3_dot, vec3Dot)
PYMEOW_PY_ALIAS(vec3_cross, vec3Cross)
PYMEOW_PY_ALIAS(vec3_normalize, vec3Normalize)
PYMEOW_PY_ALIAS(vec3_lerp, vec3Lerp)
PYMEOW_PY_ALIAS(vec3_closest, vec3Closest)

#if defined(PYMEOW_LINUX)

/* =================================================================== ptrace */
PYMEOW_PY_ALIAS(inject_syscall, ptrace::injectSyscall)
PYMEOW_PY_ALIAS(ptrace_page_protection, ptrace::pageProtection)
PYMEOW_PY_ALIAS(ptrace_allocate_memory, ptrace::allocateMemory)
PYMEOW_PY_ALIAS(ptrace_free_memory, ptrace::freeMemory)
PYMEOW_PY_ALIAS(can_trace, ptrace::canTrace)

#endif /* PYMEOW_LINUX */

} /* namespace pymeow */

#undef PYMEOW_PY_ALIAS
#undef PYMEOW_PY_ALIAS_LIST

#endif /* PYMEOW_PYCOMPAT_HPP */
