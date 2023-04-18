#ifndef INPUT_CODE_H_
#define INPUT_CODE_H_

#include <cstdint>

namespace aph
{
using KeyCodeType = uint32_t;

constexpr uint32_t APH_RELEASE = 0;
constexpr uint32_t APH_PRESS   = 1;
constexpr uint32_t APH_REPEAT  = 2;

/* The unknown key */
constexpr KeyCodeType APH_KEY_UNKNOWN = -1;

/* Printable keys */
constexpr KeyCodeType APH_KEY_SPACE         = 32;
constexpr KeyCodeType APH_KEY_APOSTROPHE    = 39; /* ' */
constexpr KeyCodeType APH_KEY_COMMA         = 44; /* , */
constexpr KeyCodeType APH_KEY_MINUS         = 45; /* - */
constexpr KeyCodeType APH_KEY_PERIOD        = 46; /* . */
constexpr KeyCodeType APH_KEY_SLASH         = 47; /* / */
constexpr KeyCodeType APH_KEY_0             = 48;
constexpr KeyCodeType APH_KEY_1             = 49;
constexpr KeyCodeType APH_KEY_2             = 50;
constexpr KeyCodeType APH_KEY_3             = 51;
constexpr KeyCodeType APH_KEY_4             = 52;
constexpr KeyCodeType APH_KEY_5             = 53;
constexpr KeyCodeType APH_KEY_6             = 54;
constexpr KeyCodeType APH_KEY_7             = 55;
constexpr KeyCodeType APH_KEY_8             = 56;
constexpr KeyCodeType APH_KEY_9             = 57;
constexpr KeyCodeType APH_KEY_SEMICOLON     = 59; /* ; */
constexpr KeyCodeType APH_KEY_EQUAL         = 61; /* = */
constexpr KeyCodeType APH_KEY_A             = 65;
constexpr KeyCodeType APH_KEY_B             = 66;
constexpr KeyCodeType APH_KEY_C             = 67;
constexpr KeyCodeType APH_KEY_D             = 68;
constexpr KeyCodeType APH_KEY_E             = 69;
constexpr KeyCodeType APH_KEY_F             = 70;
constexpr KeyCodeType APH_KEY_G             = 71;
constexpr KeyCodeType APH_KEY_H             = 72;
constexpr KeyCodeType APH_KEY_I             = 73;
constexpr KeyCodeType APH_KEY_J             = 74;
constexpr KeyCodeType APH_KEY_K             = 75;
constexpr KeyCodeType APH_KEY_L             = 76;
constexpr KeyCodeType APH_KEY_M             = 77;
constexpr KeyCodeType APH_KEY_N             = 78;
constexpr KeyCodeType APH_KEY_O             = 79;
constexpr KeyCodeType APH_KEY_P             = 80;
constexpr KeyCodeType APH_KEY_Q             = 81;
constexpr KeyCodeType APH_KEY_R             = 82;
constexpr KeyCodeType APH_KEY_S             = 83;
constexpr KeyCodeType APH_KEY_T             = 84;
constexpr KeyCodeType APH_KEY_U             = 85;
constexpr KeyCodeType APH_KEY_V             = 86;
constexpr KeyCodeType APH_KEY_W             = 87;
constexpr KeyCodeType APH_KEY_X             = 88;
constexpr KeyCodeType APH_KEY_Y             = 89;
constexpr KeyCodeType APH_KEY_Z             = 90;
constexpr KeyCodeType APH_KEY_LEFT_BRACKET  = 91;  /* [ */
constexpr KeyCodeType APH_KEY_BACKSLASH     = 92;  /* \ */
constexpr KeyCodeType APH_KEY_RIGHT_BRACKET = 93;  /* ] */
constexpr KeyCodeType APH_KEY_GRAVE_ACCENT  = 96;  /* ` */
constexpr KeyCodeType APH_KEY_WORLD_1       = 161; /* non-US #1 */
constexpr KeyCodeType APH_KEY_WORLD_2       = 162; /* non-US #2 */

/* Function keys */
constexpr KeyCodeType APH_KEY_ESCAPE        = 256;
constexpr KeyCodeType APH_KEY_ENTER         = 257;
constexpr KeyCodeType APH_KEY_TAB           = 258;
constexpr KeyCodeType APH_KEY_BACKSPACE     = 259;
constexpr KeyCodeType APH_KEY_INSERT        = 260;
constexpr KeyCodeType APH_KEY_DELETE        = 261;
constexpr KeyCodeType APH_KEY_RIGHT         = 262;
constexpr KeyCodeType APH_KEY_LEFT          = 263;
constexpr KeyCodeType APH_KEY_DOWN          = 264;
constexpr KeyCodeType APH_KEY_UP            = 265;
constexpr KeyCodeType APH_KEY_PAGE_UP       = 266;
constexpr KeyCodeType APH_KEY_PAGE_DOWN     = 267;
constexpr KeyCodeType APH_KEY_HOME          = 268;
constexpr KeyCodeType APH_KEY_END           = 269;
constexpr KeyCodeType APH_KEY_CAPS_LOCK     = 280;
constexpr KeyCodeType APH_KEY_SCROLL_LOCK   = 281;
constexpr KeyCodeType APH_KEY_NUM_LOCK      = 282;
constexpr KeyCodeType APH_KEY_PRINT_SCREEN  = 283;
constexpr KeyCodeType APH_KEY_PAUSE         = 284;
constexpr KeyCodeType APH_KEY_F1            = 290;
constexpr KeyCodeType APH_KEY_F2            = 291;
constexpr KeyCodeType APH_KEY_F3            = 292;
constexpr KeyCodeType APH_KEY_F4            = 293;
constexpr KeyCodeType APH_KEY_F5            = 294;
constexpr KeyCodeType APH_KEY_F6            = 295;
constexpr KeyCodeType APH_KEY_F7            = 296;
constexpr KeyCodeType APH_KEY_F8            = 297;
constexpr KeyCodeType APH_KEY_F9            = 298;
constexpr KeyCodeType APH_KEY_F10           = 299;
constexpr KeyCodeType APH_KEY_F11           = 300;
constexpr KeyCodeType APH_KEY_F12           = 301;
constexpr KeyCodeType APH_KEY_F13           = 302;
constexpr KeyCodeType APH_KEY_F14           = 303;
constexpr KeyCodeType APH_KEY_F15           = 304;
constexpr KeyCodeType APH_KEY_F16           = 305;
constexpr KeyCodeType APH_KEY_F17           = 306;
constexpr KeyCodeType APH_KEY_F18           = 307;
constexpr KeyCodeType APH_KEY_F19           = 308;
constexpr KeyCodeType APH_KEY_F20           = 309;
constexpr KeyCodeType APH_KEY_F21           = 310;
constexpr KeyCodeType APH_KEY_F22           = 311;
constexpr KeyCodeType APH_KEY_F23           = 312;
constexpr KeyCodeType APH_KEY_F24           = 313;
constexpr KeyCodeType APH_KEY_F25           = 314;
constexpr KeyCodeType APH_KEY_KP_0          = 320;
constexpr KeyCodeType APH_KEY_KP_1          = 321;
constexpr KeyCodeType APH_KEY_KP_2          = 322;
constexpr KeyCodeType APH_KEY_KP_3          = 323;
constexpr KeyCodeType APH_KEY_KP_4          = 324;
constexpr KeyCodeType APH_KEY_KP_5          = 325;
constexpr KeyCodeType APH_KEY_KP_6          = 326;
constexpr KeyCodeType APH_KEY_KP_7          = 327;
constexpr KeyCodeType APH_KEY_KP_8          = 328;
constexpr KeyCodeType APH_KEY_KP_9          = 329;
constexpr KeyCodeType APH_KEY_KP_DECIMAL    = 330;
constexpr KeyCodeType APH_KEY_KP_DIVIDE     = 331;
constexpr KeyCodeType APH_KEY_KP_MULTIPLY   = 332;
constexpr KeyCodeType APH_KEY_KP_SUBTRACT   = 333;
constexpr KeyCodeType APH_KEY_KP_ADD        = 334;
constexpr KeyCodeType APH_KEY_KP_ENTER      = 335;
constexpr KeyCodeType APH_KEY_KP_EQUAL      = 336;
constexpr KeyCodeType APH_KEY_LEFT_SHIFT    = 340;
constexpr KeyCodeType APH_KEY_LEFT_CONTROL  = 341;
constexpr KeyCodeType APH_KEY_LEFT_ALT      = 342;
constexpr KeyCodeType APH_KEY_LEFT_SUPER    = 343;
constexpr KeyCodeType APH_KEY_RIGHT_SHIFT   = 344;
constexpr KeyCodeType APH_KEY_RIGHT_CONTROL = 345;
constexpr KeyCodeType APH_KEY_RIGHT_ALT     = 346;
constexpr KeyCodeType APH_KEY_RIGHT_SUPER   = 347;
constexpr KeyCodeType APH_KEY_MENU          = 348;

constexpr KeyCodeType APH_KEY_LAST = APH_KEY_MENU;

/*! @} */

/*! @defgroup mods Modifier key flags
 *  @brief Modifier key flags.
 *
 *  See [key input](@ref input_key) for how these are used.
 *
 *  @ingroup input
 *  @{ */

/*! @brief If this bit is set one or more Shift keys were held down.
 *
 *  If this bit is set one or more Shift keys were held down.
 */
constexpr KeyCodeType APH_MOD_SHIFT = 0x0001;
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
constexpr KeyCodeType APH_MOD_CONTROL = 0x0002;
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
constexpr KeyCodeType APH_MOD_ALT = 0x0004;
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
constexpr KeyCodeType APH_MOD_SUPER = 0x0008;
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  APH_LOCK_KEY_MODS input mode is set.
 */
constexpr KeyCodeType APH_MOD_CAPS_LOCK = 0x0010;
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  APH_LOCK_KEY_MODS input mode is set.
 */
constexpr KeyCodeType APH_MOD_NUM_LOCK = 0x0020;

/*! @} */

/*! @} */

/*! @defgroup buttons Mouse buttons
 *  @brief Mouse button IDs.
 *
 *  See [mouse button input](@ref input_mouse_button) for how these are used.
 *
 *  @ingroup input
 *  @{ */
constexpr KeyCodeType APH_MOUSE_BUTTON_1      = 0;
constexpr KeyCodeType APH_MOUSE_BUTTON_2      = 1;
constexpr KeyCodeType APH_MOUSE_BUTTON_3      = 2;
constexpr KeyCodeType APH_MOUSE_BUTTON_4      = 3;
constexpr KeyCodeType APH_MOUSE_BUTTON_5      = 4;
constexpr KeyCodeType APH_MOUSE_BUTTON_6      = 5;
constexpr KeyCodeType APH_MOUSE_BUTTON_7      = 6;
constexpr KeyCodeType APH_MOUSE_BUTTON_8      = 7;
constexpr KeyCodeType APH_MOUSE_BUTTON_LAST   = APH_MOUSE_BUTTON_8;
constexpr KeyCodeType APH_MOUSE_BUTTON_LEFT   = APH_MOUSE_BUTTON_1;
constexpr KeyCodeType APH_MOUSE_BUTTON_RIGHT  = APH_MOUSE_BUTTON_2;
constexpr KeyCodeType APH_MOUSE_BUTTON_MIDDLE = APH_MOUSE_BUTTON_3;
/*! @} */

}  // namespace aph

#endif  // INPUT_CODE_H_
