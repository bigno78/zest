#pragma once

#include <zest/raylib_wrapper.hpp>
#include <zest/tree_sitter.hpp>
#include <zest/types.hpp>


struct CursorState
{
    struct MoveState
    {
        double down_time = 0.0;
        bool active = false;
    };

    double blink_time = 0.5;

    double time = 0.0;
    bool visible = true;

    int line = 0;
    int col = 0;
    int original_col = 0;

    MoveState state_left;
    MoveState state_right;
    MoveState state_up;
    MoveState state_down;

    double initial_delay = 0.5;
    double move_rate = 0.05;
};

struct FontInfo
{
    Font font;
    int font_size;
    float char_step;
    float char_spacing;
};

struct Editor
{
    int top_left_x;
    int top_left_y;
    int width;
    int height;
    zest::Rect text_area_rect;

    float file_space_x;
    float file_space_y;
    zest::Rect view_rect;

    FontInfo font_info;

    Image text_area_image;

    float cell_width;
    float cell_height;

    bool cursorize_view = false;

    bool selecting = false;
    bool selection_valid = false;
    zest::CellPos selection_origin;
    zest::CellPos selection_current;

    zest::tree_sitter::ParserPtr parser {
        nullptr, zest::tree_sitter::delete_parser };

    zest::tree_sitter::QueryPtr queries {
        nullptr, zest::tree_sitter::delete_query };

    zest::tree_sitter::QueryCursorPtr query_cursor {
        nullptr, zest::tree_sitter::delete_query_cursor };

};

struct App
{
    Editor editor;
    CursorState cursor;
};

App init_app(int window_width, int window_height);
