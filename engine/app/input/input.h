#ifndef APH_INPUT_H
#define APH_INPUT_H

namespace aph
{
enum class Key
{
    Unknown,
    A,
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
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    _0,
    Count
};

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

}  // namespace aph

#endif
