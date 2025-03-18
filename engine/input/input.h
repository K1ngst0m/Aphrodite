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
