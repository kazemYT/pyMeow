/*
 * pyMeow C++ -- shapes, text & textures implementation (port of render.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/render.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#  include "pymeow/error.hpp"

#  include <cstring>
#  include <unordered_map>

namespace pymeow {

namespace {

std::unordered_map<int, ::Font> gFonts;

const ::Font& requireFont(int fontId) {
    const auto it = gFonts.find(fontId);
    if (it == gFonts.end()) throw NotFoundError("Unknown Font ID: " + std::to_string(fontId));
    return it->second;
}

/* raylib >= 5.5 moved the line thickness of DrawRectangleRoundedLines into
 * DrawRectangleRoundedLinesEx. */
#  if defined(RAYLIB_VERSION_MAJOR) && defined(RAYLIB_VERSION_MINOR) && \
      (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5))
#    define PYMEOW_RAYLIB_ROUNDED_LINES_EX 1
#  endif

} /* namespace */

/* ------------------------------------------------------------------ drawing */
void drawFps(float posX, float posY) { ::DrawFPS(static_cast<int>(posX), static_cast<int>(posY)); }

void drawText(const std::string& text, float posX, float posY, float fontSize, Color color) {
    ::DrawText(text.c_str(), static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(fontSize), color);
}

void drawPixel(float posX, float posY, Color color) { ::DrawPixel(static_cast<int>(posX), static_cast<int>(posY), color); }

void drawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color, float thick) {
    ::DrawLineEx(::Vector2{startPosX, startPosY}, ::Vector2{endPosX, endPosY}, thick, color);
}

void drawCircle(float centerX, float centerY, float radius, Color color) {
    ::DrawCircleV(::Vector2{centerX, centerY}, radius, color);
}

void drawCircleLines(float centerX, float centerY, float radius, Color color) {
    ::DrawCircleLinesV(::Vector2{centerX, centerY}, radius, color);
}

void drawCircleSector(float centerX, float centerY, float radius, float startAngle, float endAngle, int segments, Color color) {
    ::DrawCircleSector(::Vector2{centerX, centerY}, radius, startAngle, endAngle, segments, color);
}

void drawCircleSectorLines(float centerX, float centerY, float radius, float startAngle, float endAngle, int segments,
                           Color color) {
    ::DrawCircleSectorLines(::Vector2{centerX, centerY}, radius, startAngle, endAngle, segments, color);
}

void drawRing(float centerX, float centerY, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments,
              Color color) {
    ::DrawRing(::Vector2{centerX, centerY}, innerRadius, outerRadius, startAngle, endAngle, segments, color);
}

void drawRingLines(float centerX, float centerY, float innerRadius, float outerRadius, float startAngle, float endAngle,
                   int segments, Color color) {
    ::DrawRingLines(::Vector2{centerX, centerY}, innerRadius, outerRadius, startAngle, endAngle, segments, color);
}

void drawEllipse(float centerX, float centerY, float radiusH, float radiusV, Color color) {
    ::DrawEllipse(static_cast<int>(centerX), static_cast<int>(centerY), radiusH, radiusV, color);
}

void drawEllipseLines(float centerX, float centerY, float radiusH, float radiusV, Color color) {
    ::DrawEllipseLines(static_cast<int>(centerX), static_cast<int>(centerY), radiusH, radiusV, color);
}

Rect drawRectangle(float posX, float posY, float width, float height, Color color) {
    const Rect rec(posX, posY, width, height);
    ::DrawRectangleRec(rec, color);
    return rec;
}

Rect drawRectangleLines(float posX, float posY, float width, float height, Color color, float lineThick) {
    const Rect rec(posX, posY, width, height);
    ::DrawRectangleLinesEx(rec, lineThick, color);
    return rec;
}

Rect drawRectangleRounded(float posX, float posY, float width, float height, float roundness, int segments, Color color) {
    const Rect rec(posX, posY, width, height);
    ::DrawRectangleRounded(rec, roundness, segments, color);
    return rec;
}

Rect drawRectangleRoundedLines(float posX, float posY, float width, float height, float roundness, int segments, Color color,
                               float lineThick) {
    const Rect rec(posX, posY, width, height);
#  if defined(PYMEOW_RAYLIB_ROUNDED_LINES_EX)
    ::DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
#  else
    ::DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
#  endif
    return rec;
}

void drawTriangle(float pos1X, float pos1Y, float pos2X, float pos2Y, float pos3X, float pos3Y, Color color) {
    ::DrawTriangle(::Vector2{pos1X, pos1Y}, ::Vector2{pos2X, pos2Y}, ::Vector2{pos3X, pos3Y}, color);
}

void drawTriangleLines(float pos1X, float pos1Y, float pos2X, float pos2Y, float pos3X, float pos3Y, Color color) {
    ::DrawTriangleLines(::Vector2{pos1X, pos1Y}, ::Vector2{pos2X, pos2Y}, ::Vector2{pos3X, pos3Y}, color);
}

void drawPoly(float posX, float posY, int sides, float radius, float rotation, Color color) {
    ::DrawPoly(::Vector2{posX, posY}, sides, radius, rotation, color);
}

void drawPolyLines(float posX, float posY, int sides, float radius, float rotation, float lineThick, Color color) {
    ::DrawPolyLinesEx(::Vector2{posX, posY}, sides, radius, rotation, lineThick, color);
}

/* ----------------------------------------------------------------- textures */
::Texture2D loadTexture(const std::string& fileName) {
    const ::Texture2D texture = ::LoadTexture(fileName.c_str());
    if (texture.id == 0) throw Error("loadTexture failed: " + fileName);
    return texture;
}

::Texture2D loadTextureBytes(const std::string& fileType, const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) throw InvalidArgumentError("loadTextureBytes: empty data");
    std::string extension = fileType;
    if (!extension.empty() && extension[0] != '.') extension = "." + extension;
    ::Image image = ::LoadImageFromMemory(extension.c_str(), data, static_cast<int>(size));
    if (image.data == nullptr) throw Error("loadTextureBytes: unable to decode '" + fileType + "' data");
    const ::Texture2D texture = ::LoadTextureFromImage(image);
    ::UnloadImage(image);
    return texture;
}

::Texture2D loadTextureBytes(const std::string& fileType, const std::vector<std::uint8_t>& data) {
    return loadTextureBytes(fileType, data.empty() ? nullptr : data.data(), data.size());
}

void drawTexture(const ::Texture2D& texture, float posX, float posY, Color tint, float rotation, float scale) {
    ::DrawTextureEx(texture, ::Vector2{posX, posY}, rotation, scale, tint);
}

void drawTexture(const ::Texture2D& texture, Rect source, Rect dest, Color tint) {
    ::DrawTexturePro(texture, source, dest, ::Vector2{0.0f, 0.0f}, 0.0f, tint);
}

void unloadTexture(::Texture2D& texture) {
    if (texture.id != 0) {
        ::UnloadTexture(texture);
        texture.id = 0;
    }
}

/* -------------------------------------------------------------------- fonts */
void loadFont(const std::string& fileName, int fontId, int fontSize) {
    ::Font font = (fontSize > 0) ? ::LoadFontEx(fileName.c_str(), fontSize, nullptr, 0) : ::LoadFont(fileName.c_str());
    if (font.texture.id == 0) throw Error("loadFont failed: " + fileName);
    gFonts[fontId] = font;
}

void unloadFont(int fontId) {
    const auto it = gFonts.find(fontId);
    if (it == gFonts.end()) return;
    ::UnloadFont(it->second);
    gFonts.erase(it);
}

bool fontExists(int fontId) { return gFonts.find(fontId) != gFonts.end(); }

void drawFont(int fontId, const std::string& text, float posX, float posY, float fontSize, float spacing, Color tint) {
    ::DrawTextEx(requireFont(fontId), text.c_str(), ::Vector2{posX, posY}, fontSize, spacing, tint);
}

Vec2 measureFont(int fontId, const std::string& text, float fontSize, float spacing) {
    return Vec2(::MeasureTextEx(requireFont(fontId), text.c_str(), fontSize, spacing));
}

int measureText(const std::string& text, int fontSize) { return ::MeasureText(text.c_str(), fontSize); }

/* --------------------------------------------------------------- collisions */
bool checkCollisionPointRec(float pointX, float pointY, Rect rec) {
    return ::CheckCollisionPointRec(::Vector2{pointX, pointY}, rec);
}

bool checkCollisionRecs(Rect rec1, Rect rec2) { return ::CheckCollisionRecs(rec1, rec2); }

bool checkCollisionCircleRec(float posX, float posY, float radius, Rect rec) {
    return ::CheckCollisionCircleRec(::Vector2{posX, posY}, radius, rec);
}

bool checkCollisionCircles(float pos1X, float pos1Y, float radius1, float pos2X, float pos2Y, float radius2) {
    return ::CheckCollisionCircles(::Vector2{pos1X, pos1Y}, radius1, ::Vector2{pos2X, pos2Y}, radius2);
}

Vec2 checkCollisionLines(float startPos1X, float startPos1Y, float endPos1X, float endPos1Y, float startPos2X, float startPos2Y,
                         float endPos2X, float endPos2Y) {
    ::Vector2 collision{0.0f, 0.0f};
    const bool hit = ::CheckCollisionLines(::Vector2{startPos1X, startPos1Y}, ::Vector2{endPos1X, endPos1Y},
                                           ::Vector2{startPos2X, startPos2Y}, ::Vector2{endPos2X, endPos2Y}, &collision);
    return hit ? Vec2(collision) : Vec2(0.0f, 0.0f);
}

/* --------------------------------------------------------- world to screen */
Vec2 worldToScreen(const float* matrix, Vec3 pos, int algo) {
    return worldToScreen(matrix, pos, ::GetScreenWidth(), ::GetScreenHeight(), algo);
}

WtsResult worldToScreenNoExc(const float* matrix, Vec3 pos, int algo) {
    return worldToScreenNoExc(matrix, pos, ::GetScreenWidth(), ::GetScreenHeight(), algo);
}

Vec2 worldToScreen(const Matrix& matrix, Vec3 pos, int algo) { return worldToScreen(matrix.data(), pos, algo); }

WtsResult worldToScreenNoExc(const Matrix& matrix, Vec3 pos, int algo) { return worldToScreenNoExc(matrix.data(), pos, algo); }

} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */
