#pragma once

#include <types.hpp>

#include <map>
#include <string_view>

namespace zest
{

namespace highlight
{

inline std::map<std::string_view, zest::Color> highlights = {
    { "keyword", zest::Color{ 255, 0, 0, 255 } },
    { "bool.constant", zest::Color{ 255, 0, 255, 255 } },
    { "function", zest::Color{ 0, 255, 0, 255 } }
};

} // namespace highlight

} // namespace zest
