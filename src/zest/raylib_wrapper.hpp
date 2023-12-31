#pragma once

#include "raylib.h"

#include <zest/types.hpp>

namespace zest
{

inline Vec2 zestify(Vector2 vec)
{
    return { vec.x, vec.y };
}

namespace raylib
{

inline ::Color to_raylib(Color color)
{
    return { color.r, color.g, color.b, color.a };
}

inline void draw_rectangle(Image& image, Rect rect, ::Color color)
{
    ImageDrawRectangle(&image, rect.x, rect.y, rect.width, rect.height, color);
}

} // namespace raylib

} // namespace zest
