/*
 * pyMeow C++ -- immediate mode GUI implementation (port of gui.nim)
 * https://github.com/qb-0/pyMeow
 */
/* Define PYMEOW_GUI_IMPLEMENTATION to compile raygui inside this translation
 * unit (it must be set before raygui.h is included). It also makes raygui's
 * internal, static GuiScrollBar() reachable for gui::scrollBar(). */
#if defined(PYMEOW_GUI_IMPLEMENTATION) && !defined(RAYGUI_IMPLEMENTATION)
#  define RAYGUI_IMPLEMENTATION
#endif

#include "pymeow/gui.hpp"

#if defined(PYMEOW_WITH_RAYLIB) && defined(PYMEOW_WITH_GUI)

#  include "pymeow/error.hpp"

#  include <algorithm>
#  include <cstring>
#  include <unordered_map>
#  include <vector>

#  if defined(RAYGUI_VERSION_MAJOR) && (RAYGUI_VERSION_MAJOR >= 5)
#    define PYMEOW_RAYGUI_5 1
#  endif

namespace pymeow {
namespace gui {

namespace {

struct DropdownState {
    std::string text;
    int active = 0;
    bool editMode = false;
};

struct TextBoxState {
    std::string text;
    std::size_t capacity = 64;
    bool editMode = false;
};

struct ColorPickerState {
    Color color = colors::white;
};

struct SpinnerState {
    int value = 0;
    bool editMode = false;
};

std::unordered_map<int, DropdownState> gDropdowns;
std::unordered_map<int, TextBoxState> gTextBoxes;
std::unordered_map<int, ColorPickerState> gColorPickers;
std::unordered_map<int, SpinnerState> gSpinners;

} /* namespace */

/* ------------------------------------------------------------- containers */
bool windowBox(Rect bounds, const std::string& title) { return ::GuiWindowBox(bounds, title.c_str()) != 0; }

void groupBox(Rect bounds, const std::string& text) { ::GuiGroupBox(bounds, text.c_str()); }

void line(Rect bounds, const std::string& text) { ::GuiLine(bounds, text.empty() ? nullptr : text.c_str()); }

void panel(Rect bounds, const std::string& text) {
#  if defined(RAYGUI_VERSION_MAJOR) && (RAYGUI_VERSION_MAJOR >= 4)
    ::GuiPanel(bounds, text.empty() ? nullptr : text.c_str());
#  else
    (void)text;
    ::GuiPanel(bounds);
#  endif
}

Rect scrollPanel(Rect bounds, const std::string& text, Rect content, Vec2& scroll) {
    ::Vector2 scrollValue = scroll;
    ::Rectangle view;
    std::memset(&view, 0, sizeof(view));
    ::GuiScrollPanel(bounds, text.empty() ? nullptr : text.c_str(), content, &scrollValue, &view);
    scroll = Vec2(scrollValue);
    return Rect(view);
}

/* ---------------------------------------------------------------- controls */
void label(Rect bounds, const std::string& text) { ::GuiLabel(bounds, text.c_str()); }

bool button(Rect bounds, const std::string& text) { return ::GuiButton(bounds, text.c_str()) != 0; }

bool labelButton(Rect bounds, const std::string& text) { return ::GuiLabelButton(bounds, text.c_str()) != 0; }

bool toggle(Rect bounds, const std::string& text, bool active) {
    bool value = active;
    ::GuiToggle(bounds, text.c_str(), &value);
    return value;
}

bool checkBox(Rect bounds, const std::string& text, bool checked) {
    bool value = checked;
    ::GuiCheckBox(bounds, text.c_str(), &value);
    return value;
}

int comboBox(Rect bounds, const std::string& text, int active) {
    int value = active;
    ::GuiComboBox(bounds, text.c_str(), &value);
    return value;
}

int dropdownBox(Rect bounds, const std::string& text, int id, int active) {
    auto it = gDropdowns.find(id);
    if (it == gDropdowns.end()) {
        DropdownState state;
        state.text = text;
        state.active = active;
        it = gDropdowns.emplace(id, state).first;
    }
    DropdownState& state = it->second;
    if (!text.empty() && state.text != text) state.text = text; /* the item list may change at runtime */

    if (::GuiDropdownBox(bounds, state.text.c_str(), &state.active, state.editMode) != 0) {
        state.editMode = !state.editMode;
        if (state.editMode)
            ::GuiLock();
        else
            ::GuiUnlock();
    }
    return state.active;
}

std::string textBox(Rect bounds, const std::string& text, int id) {
    auto it = gTextBoxes.find(id);
    if (it == gTextBoxes.end()) {
        TextBoxState state;
        state.text = text;
        state.capacity = std::max<std::size_t>(64, text.size() + static_cast<std::size_t>(std::max(0.0f, bounds.width)) + 1);
        it = gTextBoxes.emplace(id, state).first;
    }
    TextBoxState& state = it->second;

    std::vector<char> buffer(state.capacity + 1, '\0');
    std::memcpy(buffer.data(), state.text.c_str(), std::min(state.text.size(), state.capacity));
    if (::GuiTextBox(bounds, buffer.data(), static_cast<int>(state.capacity), state.editMode) != 0)
        state.editMode = !state.editMode;
    state.text.assign(buffer.data());
    return state.text;
}

int spinner(Rect bounds, const std::string& text, int value, int minValue, int maxValue, int id) {
    auto it = gSpinners.find(id);
    if (it == gSpinners.end()) it = gSpinners.emplace(id, SpinnerState{value, false}).first;
    SpinnerState& state = it->second;
    if (::GuiSpinner(bounds, text.c_str(), &state.value, minValue, maxValue, state.editMode) != 0)
        state.editMode = !state.editMode;
    return state.value;
}

float slider(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue, float maxValue) {
    float current = value;
    ::GuiSlider(bounds, textLeft.empty() ? nullptr : textLeft.c_str(), textRight.empty() ? nullptr : textRight.c_str(), &current,
                minValue, maxValue);
    return current;
}

float sliderBar(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue,
                float maxValue) {
    float current = value;
    ::GuiSliderBar(bounds, textLeft.empty() ? nullptr : textLeft.c_str(), textRight.empty() ? nullptr : textRight.c_str(),
                   &current, minValue, maxValue);
    return current;
}

float progressBar(Rect bounds, const std::string& textLeft, const std::string& textRight, float value, float minValue,
                  float maxValue) {
    float current = value;
    ::GuiProgressBar(bounds, textLeft.empty() ? nullptr : textLeft.c_str(), textRight.empty() ? nullptr : textRight.c_str(),
                     &current, minValue, maxValue);
    return current;
}

void statusBar(Rect bounds, const std::string& text) { ::GuiStatusBar(bounds, text.c_str()); }

int messageBox(Rect bounds, const std::string& title, const std::string& message, const std::string& buttons) {
#  if defined(PYMEOW_RAYGUI_5)
    int active = -1;
    ::GuiMessageBox(bounds, title.c_str(), message.c_str(), buttons.c_str(), &active);
    return active;
#  else
    return ::GuiMessageBox(bounds, title.c_str(), message.c_str(), buttons.c_str());
#  endif
}

Color colorPicker(Rect bounds, const std::string& text, int id) {
    auto it = gColorPickers.find(id);
    if (it == gColorPickers.end()) it = gColorPickers.emplace(id, ColorPickerState{colors::white}).first;
    ::Color value = it->second.color;
    ::GuiColorPicker(bounds, text.empty() ? nullptr : text.c_str(), &value);
    it->second.color = Color(value);
    return it->second.color;
}

Color colorPicker(Rect bounds, int id) { return colorPicker(bounds, std::string(), id); }

float colorBarAlpha(Rect bounds, float alpha, const std::string& text) {
    float current = alpha;
    ::GuiColorBarAlpha(bounds, text.empty() ? nullptr : text.c_str(), &current);
    return current;
}

float colorBarHue(Rect bounds, float value, const std::string& text) {
    float current = value;
    ::GuiColorBarHue(bounds, text.empty() ? nullptr : text.c_str(), &current);
    return current;
}

/* ------------------------------------------------------------------ global */
void loadStyle(const std::string& fileName) { ::GuiLoadStyle(fileName.c_str()); }
void setFade(float alpha) { ::GuiSetAlpha(alpha); }

int scrollBar(Rect bounds, int value, int minValue, int maxValue) {
#  if defined(RAYGUI_IMPLEMENTATION)
    /* GuiScrollBar is static (internal) since raygui 3.0: it is only reachable
     * from the translation unit that compiles raygui itself. */
    return ::GuiScrollBar(bounds, value, minValue, maxValue);
#  else
    float current = static_cast<float>(value);
    ::GuiSliderBar(bounds, nullptr, nullptr, &current, static_cast<float>(minValue), static_cast<float>(maxValue));
    return static_cast<int>(current);
#  endif
}
void setState(int state) { ::GuiSetState(state); }
int getState() { return ::GuiGetState(); }
void lock() { ::GuiLock(); }
void unlock() { ::GuiUnlock(); }
void setStyle(int control, int property, int value) { ::GuiSetStyle(control, property, value); }
int getStyle(int control, int property) { return ::GuiGetStyle(control, property); }

void clearWidgets() {
    gDropdowns.clear();
    gTextBoxes.clear();
    gColorPickers.clear();
    gSpinners.clear();
}

} /* namespace gui */
} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB && PYMEOW_WITH_GUI */
