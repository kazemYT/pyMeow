/*
 * pyMeow C++ -- key codes
 * https://github.com/qb-0/pyMeow
 *
 * Windows uses virtual key codes (VK_*), Linux uses X11 key symbols (XK_*).
 * Both are plain ints in the API (keyPressed / pressKey / overlay exitKey),
 * this header only provides portable names for the most common keys.
 *
 *   #include <pymeow/keys.hpp>
 *   cfg.exitKey = pm::keys::End;   // portable "END" key
 */
#ifndef PYMEOW_KEYS_HPP
#define PYMEOW_KEYS_HPP

namespace pymeow {
namespace keys {

#if defined(PYMEOW_WINDOWS)

inline constexpr int Backspace = 0x08; /* VK_BACK     */
inline constexpr int Tab       = 0x09; /* VK_TAB      */
inline constexpr int Enter     = 0x0D; /* VK_RETURN   */
inline constexpr int Shift     = 0x10; /* VK_SHIFT    */
inline constexpr int Control   = 0x11; /* VK_CONTROL  */
inline constexpr int Alt       = 0x12; /* VK_MENU     */
inline constexpr int CapsLock  = 0x14; /* VK_CAPITAL  */
inline constexpr int Escape    = 0x1B; /* VK_ESCAPE   */
inline constexpr int Space     = 0x20; /* VK_SPACE    */
inline constexpr int PageUp    = 0x21; /* VK_PRIOR    */
inline constexpr int PageDown  = 0x22; /* VK_NEXT     */
inline constexpr int End       = 0x23; /* VK_END      */
inline constexpr int Home      = 0x24; /* VK_HOME     */
inline constexpr int Left      = 0x25; /* VK_LEFT     */
inline constexpr int Up        = 0x26; /* VK_UP       */
inline constexpr int Right     = 0x27; /* VK_RIGHT    */
inline constexpr int Down      = 0x28; /* VK_DOWN     */
inline constexpr int Insert    = 0x2D; /* VK_INSERT   */
inline constexpr int Delete    = 0x2E; /* VK_DELETE   */

inline constexpr int Num0 = 0x30, Num1 = 0x31, Num2 = 0x32, Num3 = 0x33, Num4 = 0x34;
inline constexpr int Num5 = 0x35, Num6 = 0x36, Num7 = 0x37, Num8 = 0x38, Num9 = 0x39;

inline constexpr int A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46;
inline constexpr int G = 0x47, H = 0x48, I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C;
inline constexpr int M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52;
inline constexpr int S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58;
inline constexpr int Y = 0x59, Z = 0x5A;

inline constexpr int F1  = 0x70, F2  = 0x71, F3  = 0x72, F4  = 0x73;
inline constexpr int F5  = 0x74, F6  = 0x75, F7  = 0x76, F8  = 0x77;
inline constexpr int F9  = 0x78, F10 = 0x79, F11 = 0x7A, F12 = 0x7B;

#else /* X11 key symbols */

inline constexpr int Backspace = 0xFF08; /* XK_BackSpace */
inline constexpr int Tab       = 0xFF09; /* XK_Tab       */
inline constexpr int Enter     = 0xFF0D; /* XK_Return    */
inline constexpr int Shift     = 0xFFE1; /* XK_Shift_L   */
inline constexpr int Control   = 0xFFE3; /* XK_Control_L */
inline constexpr int Alt       = 0xFFE9; /* XK_Alt_L     */
inline constexpr int CapsLock  = 0xFFE5; /* XK_Caps_Lock */
inline constexpr int Escape    = 0xFF1B; /* XK_Escape    */
inline constexpr int Space     = 0x0020; /* XK_space     */
inline constexpr int PageUp    = 0xFF55; /* XK_Page_Up   */
inline constexpr int PageDown  = 0xFF56; /* XK_Page_Down */
inline constexpr int End       = 0xFF57; /* XK_End       */
inline constexpr int Home      = 0xFF50; /* XK_Home      */
inline constexpr int Left      = 0xFF51; /* XK_Left      */
inline constexpr int Up        = 0xFF52; /* XK_Up        */
inline constexpr int Right     = 0xFF53; /* XK_Right     */
inline constexpr int Down      = 0xFF54; /* XK_Down      */
inline constexpr int Insert    = 0xFF63; /* XK_Insert    */
inline constexpr int Delete    = 0xFFFF; /* XK_Delete    */

inline constexpr int Num0 = 0x0030, Num1 = 0x0031, Num2 = 0x0032, Num3 = 0x0033, Num4 = 0x0034;
inline constexpr int Num5 = 0x0035, Num6 = 0x0036, Num7 = 0x0037, Num8 = 0x0038, Num9 = 0x0039;

inline constexpr int A = 0x0061, B = 0x0062, C = 0x0063, D = 0x0064, E = 0x0065, F = 0x0066;
inline constexpr int G = 0x0067, H = 0x0068, I = 0x0069, J = 0x006A, K = 0x006B, L = 0x006C;
inline constexpr int M = 0x006D, N = 0x006E, O = 0x006F, P = 0x0070, Q = 0x0071, R = 0x0072;
inline constexpr int S = 0x0073, T = 0x0074, U = 0x0075, V = 0x0076, W = 0x0077, X = 0x0078;
inline constexpr int Y = 0x0079, Z = 0x007A;

inline constexpr int F1  = 0xFFBE, F2  = 0xFFBF, F3  = 0xFFC0, F4  = 0xFFC1;
inline constexpr int F5  = 0xFFC2, F6  = 0xFFC3, F7  = 0xFFC4, F8  = 0xFFC5;
inline constexpr int F9  = 0xFFC6, F10 = 0xFFC7, F11 = 0xFFC8, F12 = 0xFFC9;

#endif

} /* namespace keys */
} /* namespace pymeow */

#endif /* PYMEOW_KEYS_HPP */
