#pragma once

#include <cstdint>

namespace zest
{

struct Vec2
{
    float x, y;
};

struct CellPos
{
    int line;
    int col;
};

struct Rect
{
    float x, y;
    float width, height;
};

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

inline float get_top(const Rect& rect) { return rect.y; }
inline float get_bot(const Rect& rect) { return rect.y + rect.height; }
inline float get_left(const Rect& rect) { return rect.x; }
inline float get_right(const Rect& rect) { return rect.x + rect.width; }

inline bool is_inside(Vec2 pos, const Rect& rect)
{
    return pos.x >= rect.x && pos.x <= rect.x + rect.width
            && pos.y >= rect.y && pos.y <= rect.y + rect.height;
}

inline  bool are_intersecting(const Rect& lhs, const Rect& rhs)
{
    return get_left(rhs) <= get_right(lhs) && get_right(rhs) >= get_left(lhs)
        && get_bot(rhs) >= get_top(lhs) && get_top(rhs) <= get_bot(lhs);
}

inline  Rect clip_rect_to_fit(const Rect& container, const Rect& target)
{
    Rect result = target;

    if (result.x > get_right(container) || get_right(result) < container.x
        || result.y > get_bot(container) || get_bot(result) < container.y)
    {
        return { 0, 0, 0, 0 };
    }

    if (result.x < container.x)
    {
        result.width -= container.x - result.x;
        result.x = container.x;
    }

    if (get_right(result) > get_right(container))
        result.width -= get_right(result) - get_right(container);

    if (result.y < container.y)
    {
        result.height -= container.y - result.y;
        result.y = container.y;
    }

    if (get_bot(result) > get_bot(container))
        result.height -= get_bot(result) - get_bot(container);

    return result;
}

} // namespace zest
