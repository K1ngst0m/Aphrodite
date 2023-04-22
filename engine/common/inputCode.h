#ifndef INPUT_CODE_H_
#define INPUT_CODE_H_

#include <cstdint>

namespace aph
{
using KeyId         = uint32_t;
using MouseButtonId = uint32_t;
using InputStatus   = uint32_t;
}  // namespace aph

namespace aph::input
{

constexpr InputStatus STATUS_RELEASE = 0;
constexpr InputStatus STATUS_PRESS   = 1;
constexpr InputStatus STATUS_REPEAT  = 2;

/* The unknown key */
constexpr KeyId KEY_UNKNOWN = -1;

/* Printable keys */
constexpr KeyId KEY_SPACE         = 32;
constexpr KeyId KEY_APOSTROPHE    = 39; /* ' */
constexpr KeyId KEY_COMMA         = 44; /* , */
constexpr KeyId KEY_MINUS         = 45; /* - */
constexpr KeyId KEY_PERIOD        = 46; /* . */
constexpr KeyId KEY_SLASH         = 47; /* / */
constexpr KeyId KEY_0             = 48;
constexpr KeyId KEY_1             = 49;
constexpr KeyId KEY_2             = 50;
constexpr KeyId KEY_3             = 51;
constexpr KeyId KEY_4             = 52;
constexpr KeyId KEY_5             = 53;
constexpr KeyId KEY_6             = 54;
constexpr KeyId KEY_7             = 55;
constexpr KeyId KEY_8             = 56;
constexpr KeyId KEY_9             = 57;
constexpr KeyId KEY_SEMICOLON     = 59; /* ; */
constexpr KeyId KEY_EQUAL         = 61; /* = */
constexpr KeyId KEY_A             = 65;
constexpr KeyId KEY_B             = 66;
constexpr KeyId KEY_C             = 67;
constexpr KeyId KEY_D             = 68;
constexpr KeyId KEY_E             = 69;
constexpr KeyId KEY_F             = 70;
constexpr KeyId KEY_G             = 71;
constexpr KeyId KEY_H             = 72;
constexpr KeyId KEY_I             = 73;
constexpr KeyId KEY_J             = 74;
constexpr KeyId KEY_K             = 75;
constexpr KeyId KEY_L             = 76;
constexpr KeyId KEY_M             = 77;
constexpr KeyId KEY_N             = 78;
constexpr KeyId KEY_O             = 79;
constexpr KeyId KEY_P             = 80;
constexpr KeyId KEY_Q             = 81;
constexpr KeyId KEY_R             = 82;
constexpr KeyId KEY_S             = 83;
constexpr KeyId KEY_T             = 84;
constexpr KeyId KEY_U             = 85;
constexpr KeyId KEY_V             = 86;
constexpr KeyId KEY_W             = 87;
constexpr KeyId KEY_X             = 88;
constexpr KeyId KEY_Y             = 89;
constexpr KeyId KEY_Z             = 90;
constexpr KeyId KEY_LEFT_BRACKET  = 91;  /* [ */
constexpr KeyId KEY_BACKSLASH     = 92;  /* \ */
constexpr KeyId KEY_RIGHT_BRACKET = 93;  /* ] */
constexpr KeyId KEY_GRAVE_ACCENT  = 96;  /* ` */
constexpr KeyId KEY_WORLD_1       = 161; /* non-US #1 */
constexpr KeyId KEY_WORLD_2       = 162; /* non-US #2 */

/* Function keys */
constexpr KeyId KEY_ESCAPE        = 256;
constexpr KeyId KEY_ENTER         = 257;
constexpr KeyId KEY_TAB           = 258;
constexpr KeyId KEY_BACKSPACE     = 259;
constexpr KeyId KEY_INSERT        = 260;
constexpr KeyId KEY_DELETE        = 261;
constexpr KeyId KEY_RIGHT         = 262;
constexpr KeyId KEY_LEFT          = 263;
constexpr KeyId KEY_DOWN          = 264;
constexpr KeyId KEY_UP            = 265;
constexpr KeyId KEY_PAGE_UP       = 266;
constexpr KeyId KEY_PAGE_DOWN     = 267;
constexpr KeyId KEY_HOME          = 268;
constexpr KeyId KEY_END           = 269;
constexpr KeyId KEY_CAPS_LOCK     = 280;
constexpr KeyId KEY_SCROLL_LOCK   = 281;
constexpr KeyId KEY_NUM_LOCK      = 282;
constexpr KeyId KEY_PRINT_SCREEN  = 283;
constexpr KeyId KEY_PAUSE         = 284;
constexpr KeyId KEY_F1            = 290;
constexpr KeyId KEY_F2            = 291;
constexpr KeyId KEY_F3            = 292;
constexpr KeyId KEY_F4            = 293;
constexpr KeyId KEY_F5            = 294;
constexpr KeyId KEY_F6            = 295;
constexpr KeyId KEY_F7            = 296;
constexpr KeyId KEY_F8            = 297;
constexpr KeyId KEY_F9            = 298;
constexpr KeyId KEY_F10           = 299;
constexpr KeyId KEY_F11           = 300;
constexpr KeyId KEY_F12           = 301;
constexpr KeyId KEY_F13           = 302;
constexpr KeyId KEY_F14           = 303;
constexpr KeyId KEY_F15           = 304;
constexpr KeyId KEY_F16           = 305;
constexpr KeyId KEY_F17           = 306;
constexpr KeyId KEY_F18           = 307;
constexpr KeyId KEY_F19           = 308;
constexpr KeyId KEY_F20           = 309;
constexpr KeyId KEY_F21           = 310;
constexpr KeyId KEY_F22           = 311;
constexpr KeyId KEY_F23           = 312;
constexpr KeyId KEY_F24           = 313;
constexpr KeyId KEY_F25           = 314;
constexpr KeyId KEY_KP_0          = 320;
constexpr KeyId KEY_KP_1          = 321;
constexpr KeyId KEY_KP_2          = 322;
constexpr KeyId KEY_KP_3          = 323;
constexpr KeyId KEY_KP_4          = 324;
constexpr KeyId KEY_KP_5          = 325;
constexpr KeyId KEY_KP_6          = 326;
constexpr KeyId KEY_KP_7          = 327;
constexpr KeyId KEY_KP_8          = 328;
constexpr KeyId KEY_KP_9          = 329;
constexpr KeyId KEY_KP_DECIMAL    = 330;
constexpr KeyId KEY_KP_DIVIDE     = 331;
constexpr KeyId KEY_KP_MULTIPLY   = 332;
constexpr KeyId KEY_KP_SUBTRACT   = 333;
constexpr KeyId KEY_KP_ADD        = 334;
constexpr KeyId KEY_KP_ENTER      = 335;
constexpr KeyId KEY_KP_EQUAL      = 336;
constexpr KeyId KEY_LEFT_SHIFT    = 340;
constexpr KeyId KEY_LEFT_CONTROL  = 341;
constexpr KeyId KEY_LEFT_ALT      = 342;
constexpr KeyId KEY_LEFT_SUPER    = 343;
constexpr KeyId KEY_RIGHT_SHIFT   = 344;
constexpr KeyId KEY_RIGHT_CONTROL = 345;
constexpr KeyId KEY_RIGHT_ALT     = 346;
constexpr KeyId KEY_RIGHT_SUPER   = 347;
constexpr KeyId KEY_MENU          = 348;

constexpr KeyId KEY_LAST = KEY_MENU;

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
constexpr KeyId MOD_SHIFT = 0x0001;
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
constexpr KeyId MOD_CONTROL = 0x0002;
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
constexpr KeyId MOD_ALT = 0x0004;
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
constexpr KeyId MOD_SUPER = 0x0008;
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  LOCK_KEY_MODS input mode is set.
 */
constexpr KeyId MOD_CAPS_LOCK = 0x0010;
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  LOCK_KEY_MODS input mode is set.
 */
constexpr KeyId MOD_NUM_LOCK = 0x0020;

/*! @} */

/*! @} */

/*! @defgroup buttons Mouse buttons
 *  @brief Mouse button IDs.
 *
 *  See [mouse button input](@ref input_mouse_button) for how these are used.
 *
 *  @ingroup input
 *  @{ */
constexpr MouseButtonId MOUSE_BUTTON_1      = 0;
constexpr MouseButtonId MOUSE_BUTTON_2      = 1;
constexpr MouseButtonId MOUSE_BUTTON_3      = 2;
constexpr MouseButtonId MOUSE_BUTTON_4      = 3;
constexpr MouseButtonId MOUSE_BUTTON_5      = 4;
constexpr MouseButtonId MOUSE_BUTTON_6      = 5;
constexpr MouseButtonId MOUSE_BUTTON_7      = 6;
constexpr MouseButtonId MOUSE_BUTTON_8      = 7;
constexpr MouseButtonId MOUSE_BUTTON_LAST   = MOUSE_BUTTON_8;
constexpr MouseButtonId MOUSE_BUTTON_LEFT   = MOUSE_BUTTON_1;
constexpr MouseButtonId MOUSE_BUTTON_RIGHT  = MOUSE_BUTTON_2;
constexpr MouseButtonId MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON_3;
/*! @} */

}  // namespace aph::input

#endif  // INPUT_CODE_H_
