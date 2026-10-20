/*
 * pyMeow C++ -- immediate mode GUI (port of gui.nim, wraps raygui)
 * https://github.com/qb-0/pyMeow
 *
 * Every control exists in two flavours: one taking a Rect and one taking
 * (posX, posY, width, height) like the Python API does.
 *
 * Stateful controls (dropdownBox, textBox, colorPicker, spinner) keep their
 * state in an internal table keyed by the `id` argument, exactly like the
 * Python module.
 *
 * Requires raygui (https://github.com/raysan5/raygui, raygui 4.0 or newer).
 * Define RAYGUI_IMPLEMENTATION in one of your translation units, or build
 * pyMeow with PYMEOW_GUI_IMPLEMENTATION to let pymeow provide it.
 */
#ifndef PYMEOW_GUI_HPP
#define PYMEOW_GUI_HPP

#include "common.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#if !defined(PYMEOW_NO_GUI)
#  if defined(__has_include)
#    if __has_include(<raygui.h>)
#      include <raygui.h>
#      define PYMEOW_WITH_GUI 1
#    elif __has_include("raygui.h")
#      include "raygui.h"
#      define PYMEOW_WITH_GUI 1
#    endif
#  endif
#endif

#if defined(PYMEOW_WITH_GUI)

#include "types.hpp"

#include <string>

namespace pymeow {
namespace gui {

/* ------------------------------------------------------------- containers */
PYMEOW_API bool windowBox(Rect bounds, const std::string& title); /* true when the close button was clicked */
PYMEOW_API void groupBox(Rect bounds, const std::string& text);
PYMEOW_API void line(Rect bounds, const std::string& text = "");
PYMEOW_API void panel(Rect bounds, const std::string& text = "");
PYMEOW_API Rect scrollPanel(Rect bounds, const std::string& text, Rect content, Vec2& scroll);
/* Python: gui_scroll_bar. raygui made GuiScrollBar internal in 3.0, so this
 * falls back to a slider bar when raygui is linked from a separate
 * translation unit (define PYMEOW_GUI_IMPLEMENTATION to get the real one). */
PYMEOW_API int scrollBar(Rect bounds, int value, int minValue, int maxValue);

/* ---------------------------------------------------------------- controls */
PYMEOW_API void label(Rect bounds, const std::string& text);
PYMEOW_API bool button(Rect bounds, const std::string& text);
PYMEOW_API bool labelButton(Rect bounds, const std::string& text);
PYMEOW_API bool toggle(Rect bounds, const std::string& text, bool active);
PYMEOW_API bool checkBox(Rect bounds, const std::string& text, bool checked);
/* `text` is a ";" separated list of items, e.g. "One;Two;Three". */
PYMEOW_API int comboBox(Rect bounds, const std::string& text, int active = 0);
PYMEOW_API int dropdownBox(Rect bounds, const std::string& text, int id, int active = 0);
PYMEOW_API std::string textBox(Rect bounds, const std::string& text, int id);
PYMEOW_API int spinner(Rect bounds, const std::string& text, int value, int minValue, int maxValue, int id);
PYMEOW_API float slider(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue,
                        float maxValue);
PYMEOW_API float sliderBar(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue,
                           float maxValue);
PYMEOW_API float progressBar(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue,
                             float maxValue);
PYMEOW_API void statusBar(Rect bounds, const std::string& text);
/* Returns the index of the pressed button (-1 when the message box is still open). */
PYMEOW_API int messageBox(Rect bounds, const std::string& title, const std::string& message, const std::string& buttons);
PYMEOW_API Color colorPicker(Rect bounds, int id);
PYMEOW_API Color colorPicker(Rect bounds, const std::string& text, int id);
PYMEOW_API float colorBarAlpha(Rect bounds, float alpha, const std::string& text = "");
PYMEOW_API float colorBarHue(Rect bounds, float value, const std::string& text = "");

/* ------------------------------------------------------------------ global */
PYMEOW_API void loadStyle(const std::string& fileName);
/* Python: gui_fade -- global alpha of every control (0.0 .. 1.0). */
PYMEOW_API void setFade(float alpha);
PYMEOW_API void setState(int state);
PYMEOW_API int getState();
PYMEOW_API void lock();
PYMEOW_API void unlock();
PYMEOW_API void setStyle(int control, int property, int value);
PYMEOW_API int getStyle(int control, int property);
/* Forgets the state of every dropdownBox / textBox / colorPicker / spinner. */
PYMEOW_API void clearWidgets();

/* ------------------------------------- (posX, posY, width, height) variants */
inline bool windowBox(float posX, float posY, float width, float height, const std::string& title) {
    return windowBox(Rect(posX, posY, width, height), title);
}
inline void groupBox(float posX, float posY, float width, float height, const std::string& text) {
    groupBox(Rect(posX, posY, width, height), text);
}
inline void line(float posX, float posY, float width, float height, const std::string& text) {
    line(Rect(posX, posY, width, height), text);
}
inline void panel(float posX, float posY, float width, float height, const std::string& text = "") {
    panel(Rect(posX, posY, width, height), text);
}
inline void label(float posX, float posY, float width, float height, const std::string& text) {
    label(Rect(posX, posY, width, height), text);
}
inline bool button(float posX, float posY, float width, float height, const std::string& text) {
    return button(Rect(posX, posY, width, height), text);
}
inline bool labelButton(float posX, float posY, float width, float height, const std::string& text) {
    return labelButton(Rect(posX, posY, width, height), text);
}
inline bool toggle(float posX, float posY, float width, float height, const std::string& text, bool active) {
    return toggle(Rect(posX, posY, width, height), text, active);
}
inline bool checkBox(float posX, float posY, float width, float height, const std::string& text, bool checked) {
    return checkBox(Rect(posX, posY, width, height), text, checked);
}
inline int comboBox(float posX, float posY, float width, float height, const std::string& text, int active = 0) {
    return comboBox(Rect(posX, posY, width, height), text, active);
}
inline int dropdownBox(float posX, float posY, float width, float height, const std::string& text, int id, int active = 0) {
    return dropdownBox(Rect(posX, posY, width, height), text, id, active);
}
inline std::string textBox(float posX, float posY, float width, float height, const std::string& text, int id) {
    return textBox(Rect(posX, posY, width, height), text, id);
}
inline int spinner(float posX, float posY, float width, float height, const std::string& text, int value, int minValue,
                   int maxValue, int id) {
    return spinner(Rect(posX, posY, width, height), text, value, minValue, maxValue, id);
}
inline float slider(float posX, float posY, float width, float height, const std::string& textLeft, const std::string& textRight,
                    float value, float minValue, float maxValue) {
    return slider(Rect(posX, posY, width, height), textLeft, textRight, value, minValue, maxValue);
}
inline float sliderBar(float posX, float posY, float width, float height, const std::string& textLeft,
                       const std::string& textRight, float value, float minValue, float maxValue) {
    return sliderBar(Rect(posX, posY, width, height), textLeft, textRight, value, minValue, maxValue);
}
inline float progressBar(float posX, float posY, float width, float height, const std::string& textLeft,
                         const std::string& textRight, float value, float minValue, float maxValue) {
    return progressBar(Rect(posX, posY, width, height), textLeft, textRight, value, minValue, maxValue);
}
inline void statusBar(float posX, float posY, float width, float height, const std::string& text) {
    statusBar(Rect(posX, posY, width, height), text);
}
inline int messageBox(float posX, float posY, float width, float height, const std::string& title, const std::string& message,
                      const std::string& buttons) {
    return messageBox(Rect(posX, posY, width, height), title, message, buttons);
}
inline Color colorPicker(float posX, float posY, float width, float height, int id) {
    return colorPicker(Rect(posX, posY, width, height), id);
}
inline Color colorPicker(float posX, float posY, float width, float height, const std::string& text, int id) {
    return colorPicker(Rect(posX, posY, width, height), text, id);
}
inline int scrollBar(float posX, float posY, float width, float height, int value, int minValue, int maxValue) {
    return scrollBar(Rect(posX, posY, width, height), value, minValue, maxValue);
}
inline float colorBarAlpha(float posX, float posY, float width, float height, float alpha, const std::string& text = "") {
    return colorBarAlpha(Rect(posX, posY, width, height), alpha, text);
}
inline float colorBarHue(float posX, float posY, float width, float height, float value, const std::string& text = "") {
    return colorBarHue(Rect(posX, posY, width, height), value, text);
}

} /* namespace gui */
} /* namespace pymeow */

#endif /* PYMEOW_WITH_GUI */

#endif /* PYMEOW_WITH_RAYLIB */

#endif /* PYMEOW_GUI_HPP */
