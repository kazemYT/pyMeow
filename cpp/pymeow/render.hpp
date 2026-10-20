/*
 * pyMeow C++ -- shapes, text & textures (port of render.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Thin C++ wrappers around the raylib 2D drawing calls, with the same names the
 * Python module uses (draw_line, draw_rectangle_lines, ...).
 * Requires raylib. Not compiled when PYMEOW_NO_GRAPHICS is defined.
 */
#ifndef PYMEOW_RENDER_HPP
#define PYMEOW_RENDER_HPP

#include "common.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#include "types.hpp"
#include "utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace pymeow {

/* ------------------------------------------------------------------ drawing */
PYMEOW_API void drawFps(float posX, float posY);
PYMEOW_API void drawText(const std::string& text, float posX, float posY, float fontSize, Color color);
PYMEOW_API void drawPixel(float posX, float posY, Color color);
PYMEOW_API void drawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color, float thick = 1.0f);

PYMEOW_API void drawCircle(float centerX, float centerY, float radius, Color color);
PYMEOW_API void drawCircleLines(float centerX, float centerY, float radius, Color color);
PYMEOW_API void drawCircleSector(float centerX, float centerY, float radius, float startAngle, float endAngle, int segments,
                                 Color color);
PYMEOW_API void drawCircleSectorLines(float centerX, float centerY, float radius, float startAngle, float endAngle, int segments,
                                      Color color);
PYMEOW_API void drawRing(float centerX, float centerY, float innerRadius, float outerRadius, float startAngle, float endAngle,
                         int segments, Color color);
PYMEOW_API void drawRingLines(float centerX, float centerY, float innerRadius, float outerRadius, float startAngle,
                              float endAngle, int segments, Color color);
PYMEOW_API void drawEllipse(float centerX, float centerY, float radiusH, float radiusV, Color color);
PYMEOW_API void drawEllipseLines(float centerX, float centerY, float radiusH, float radiusV, Color color);

PYMEOW_API Rect drawRectangle(float posX, float posY, float width, float height, Color color);
PYMEOW_API Rect drawRectangleLines(float posX, float posY, float width, float height, Color color, float lineThick = 1.0f);
PYMEOW_API Rect drawRectangleRounded(float posX, float posY, float width, float height, float roundness, int segments,
                                     Color color);
PYMEOW_API Rect drawRectangleRoundedLines(float posX, float posY, float width, float height, float roundness, int segments,
                                          Color color, float lineThick = 1.0f);

PYMEOW_API void drawTriangle(float pos1X, float pos1Y, float pos2X, float pos2Y, float pos3X, float pos3Y, Color color);
PYMEOW_API void drawTriangleLines(float pos1X, float pos1Y, float pos2X, float pos2Y, float pos3X, float pos3Y, Color color);
PYMEOW_API void drawPoly(float posX, float posY, int sides, float radius, float rotation, Color color);
PYMEOW_API void drawPolyLines(float posX, float posY, int sides, float radius, float rotation, float lineThick, Color color);

/* ----------------------------------------------------------------- textures */
PYMEOW_API ::Texture2D loadTexture(const std::string& fileName);
/* fileType is the extension of the encoded data, e.g. "png". */
PYMEOW_API ::Texture2D loadTextureBytes(const std::string& fileType, const std::vector<std::uint8_t>& data);
PYMEOW_API ::Texture2D loadTextureBytes(const std::string& fileType, const std::uint8_t* data, std::size_t size);
PYMEOW_API void drawTexture(const ::Texture2D& texture, float posX, float posY, Color tint, float rotation = 0.0f,
                            float scale = 1.0f);
PYMEOW_API void drawTexture(const ::Texture2D& texture, Rect source, Rect dest, Color tint);
PYMEOW_API void unloadTexture(::Texture2D& texture);

/* -------------------------------------------------------------------- fonts */
PYMEOW_API void loadFont(const std::string& fileName, int fontId, int fontSize = 0);
PYMEOW_API void unloadFont(int fontId);
PYMEOW_API bool fontExists(int fontId);
/* Throws NotFoundError for an unknown font id (like the Python module). */
PYMEOW_API void drawFont(int fontId, const std::string& text, float posX, float posY, float fontSize, float spacing, Color tint);
PYMEOW_API Vec2 measureFont(int fontId, const std::string& text, float fontSize, float spacing);

/* -------------------------------------------------------------------- text */
PYMEOW_API int measureText(const std::string& text, int fontSize);

/* --------------------------------------------------------------- collisions */
PYMEOW_API bool checkCollisionPointRec(float pointX, float pointY, Rect rec);
PYMEOW_API bool checkCollisionRecs(Rect rec1, Rect rec2);
PYMEOW_API bool checkCollisionCircleRec(float posX, float posY, float radius, Rect rec);
PYMEOW_API bool checkCollisionCircles(float pos1X, float pos1Y, float radius1, float pos2X, float pos2Y, float radius2);
/* Returns the intersection point, or (0, 0) when the lines do not intersect. */
PYMEOW_API Vec2 checkCollisionLines(float startPos1X, float startPos1Y, float endPos1X, float endPos1Y, float startPos2X,
                                    float startPos2Y, float endPos2X, float endPos2Y);

/* --------------------------------------------------------- world to screen */
/* Same projection as in utils.hpp but uses the current raylib screen size. */
PYMEOW_API Vec2 worldToScreen(const Matrix& matrix, Vec3 pos, int algo = 0);
PYMEOW_API WtsResult worldToScreenNoExc(const Matrix& matrix, Vec3 pos, int algo = 0);
PYMEOW_API Vec2 worldToScreen(const float* matrix, Vec3 pos, int algo = 0);
PYMEOW_API WtsResult worldToScreenNoExc(const float* matrix, Vec3 pos, int algo = 0);

} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */

#endif /* PYMEOW_RENDER_HPP */
