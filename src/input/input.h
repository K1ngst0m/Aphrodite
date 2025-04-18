#pragma once

namespace aph
{
enum class Key
{
    Unknown,
    _0 = 48,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    A = 65,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Return,
    LeftCtrl,
    LeftAlt,
    LeftShift,
    Space,
    Escape,
    Left,
    Right,
    Up,
    Down,
    F1 = 112,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Count
};

inline std::string KeyToStr(Key key)
{
    if ((key >= Key::A && key <= Key::Z) || (key >= Key::_0 && key <= Key::_9))
    {
        char keyChar = static_cast<char>(key);
        return { 1, keyChar };
    }

    switch (key)
    {
    case Key::Unknown:
        return "Unknown";
    case Key::Return:
        return "Return";
    case Key::LeftCtrl:
        return "Left Ctrl";
    case Key::LeftAlt:
        return "Left Alt";
    case Key::LeftShift:
        return "Left Shift";
    case Key::Space:
        return "Space";
    case Key::Escape:
        return "Escape";
    case Key::Left:
        return "Left Arrow";
    case Key::Right:
        return "Right Arrow";
    case Key::Up:
        return "Up Arrow";
    case Key::Down:
        return "Down Arrow";
    case Key::F1:
        return "F1";
    case Key::F2:
        return "F2";
    case Key::F3:
        return "F3";
    case Key::F4:
        return "F4";
    case Key::F5:
        return "F5";
    case Key::F6:
        return "F6";
    case Key::F7:
        return "F7";
    case Key::F8:
        return "F8";
    case Key::F9:
        return "F9";
    case Key::F10:
        return "F10";
    case Key::F11:
        return "F11";
    case Key::F12:
        return "F12";
    case Key::Count:
        return "Count";
    default:
        return "Invalid Key";
    }
}

enum class MouseButton
{
    Left,
    Middle,
    Right,
    Count
};

enum class KeyState
{
    Pressed,
    Released,
    Repeat,
    Count
};

} // namespace aph
