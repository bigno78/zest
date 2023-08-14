#pragma once

namespace zest
{

namespace highlight
{

inline const char* cpp_queries = R"(
    (true) @bool.constant
    (false) @bool.constant

    [
        "if"
        "else"
        "for"
        "while"
        "break"
        "continue"
        "class"
    ] @keyword

    (function_definition
        declarator: (function_declarator
            declarator: (identifier) @function ))
)";

} // namespace highlight

} // namespace zest
