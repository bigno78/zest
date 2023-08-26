#include <zest/app.hpp>
#include <zest/raylib_wrapper.hpp>
#include <zest/text.hpp>
#include <zest/tree_sitter.hpp>
#include <zest/types.hpp>
#include <zest/highlight/captures.hpp>
#include <zest/highlight/queries.hpp>

#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>


zest::CellPos window_to_cursor_pos(Editor& editor,
                                   LineBuffer& line_buffer,
                                   zest::Vec2 pos)
{
    pos.x += editor.file_space_x - editor.top_left_x;
    pos.y += editor.file_space_y - editor.top_left_y;

    int col = std::round(pos.x/editor.cell_width);
    int row = pos.y/editor.cell_height;

    if (col > line_buffer.get_line(row).size())
        col = line_buffer.get_line(row).size();

    if (row >= line_buffer.line_count())
        row = line_buffer.line_count();

    return { row, col };
}

zest::CellPos window_to_cell_pos(Editor& editor,
                                 LineBuffer& line_buffer,
                                 zest::Vec2 pos)
{
    pos.x += editor.file_space_x - editor.top_left_x;
    pos.y += editor.file_space_y - editor.top_left_y;

    int col = pos.x/editor.cell_width;
    int row = pos.y/editor.cell_height;

    if (col > line_buffer.get_line(row).size())
        col = line_buffer.get_line(row).size();

    if (row >= line_buffer.line_count())
        row = line_buffer.line_count();

    return { row, col };
}

bool move_cursor_up(CursorState& cursor, const LineBuffer& line_buffer)
{
    if (cursor.line == 0)
        return false;

    cursor.line--;
    cursor.col = std::min((size_t)cursor.original_col,
                          line_buffer.get_line(cursor.line).size());

    return true;
}

bool move_cursor_down(CursorState& cursor, const LineBuffer& line_buffer)
{
    if (cursor.line > line_buffer.line_count() - 1)
        return false;

    cursor.line++;
    cursor.col = std::min((size_t)cursor.original_col,
                          line_buffer.get_line(cursor.line).size());

    return true;
}

bool move_cursor_left(CursorState& cursor, const LineBuffer& line_buffer)
{
    if (cursor.col == 0)
    {
        bool has_moved = move_cursor_up(cursor, line_buffer);
        if (has_moved)
        {
            cursor.col = line_buffer.get_line(cursor.line).size();
            cursor.original_col = cursor.col;
        }
        return has_moved;
    }

    cursor.col--;
    cursor.original_col = cursor.col;

    return true;
}

bool move_cursor_right(CursorState& cursor, const LineBuffer& line_buffer)
{
    if (cursor.col == line_buffer.get_line(cursor.line).size())
    {
        bool has_moved = move_cursor_down(cursor, line_buffer);
        if (has_moved)
        {
            cursor.col = 0;
            cursor.original_col = cursor.col;
        }
        return has_moved;
    }

    cursor.col++;
    cursor.original_col = cursor.col;

    return true;
}

void stop_cursor(CursorState& cursor)
{
    cursor.state_down.active = false;
    cursor.state_up.active = false;
    cursor.state_right.active = false;
    cursor.state_left.active = false;
}

template<typename MoveFunc>
void update_cursor_direction(LineBuffer& line_buffer,
                             CursorState& cursor,
                             Editor& editor,
                             MoveFunc move, int key,
                             CursorState::MoveState& state,
                             double time_delta)
{
    auto move_cursor = [&] ()
    {
        bool has_moved = move(cursor, line_buffer);
        if (has_moved)
        {
            cursor.time = 0;
            cursor.visible = true;
        }
    };

    if (IsKeyPressed(key))
    {
        stop_cursor(cursor);
        move_cursor();
        state.down_time = cursor.move_rate - cursor.initial_delay;
        state.active = true;

        editor.cursorize_view = true;
    }
    else if (IsKeyDown(key) && state.active)
    {
        state.down_time += time_delta;
        if (state.down_time >= cursor.move_rate)
        {
            move_cursor();
            state.down_time -= cursor.move_rate;
        }

        editor.cursorize_view = true;
    }
}

void set_cursor_to_mouse(CursorState& cursor,
                         Editor& editor,
                         LineBuffer& line_buffer)
{
    zest::Vec2 mouse_pos = zest::zestify(GetMousePosition());

    if (!zest::is_inside(mouse_pos, editor.text_area_rect))
        return;

    zest::CellPos cell_pos = window_to_cursor_pos(editor, line_buffer,
                                                  mouse_pos);

    cursor.col = cell_pos.col;
    cursor.original_col = cursor.col;
    cursor.line = cell_pos.line;

    cursor.visible = true;
    cursor.time = 0;
}

void set_file_view_to_cursor(CursorState& cursor, Editor& editor)
{
    zest::Rect cursor_cell = {
        float(cursor.col*editor.font_info.char_step),
        float(cursor.line*editor.font_info.font_size),
        editor.font_info.char_step,
        float(editor.font_info.font_size)
    };

    if (zest::get_top(cursor_cell) < editor.file_space_y)
        editor.file_space_y = cursor_cell.y;

    if (zest::get_bot(cursor_cell) > editor.file_space_y + editor.height)
        editor.file_space_y = zest::get_bot(cursor_cell) - editor.height;

    if (zest::get_left(cursor_cell) < editor.file_space_x)
        editor.file_space_x = zest::get_left(cursor_cell);

    if (zest::get_right(cursor_cell) > editor.file_space_x + editor.width)
        editor.file_space_x = zest::get_right(cursor_cell) - editor.width;

    editor.view_rect.x = editor.file_space_x;
    editor.view_rect.y = editor.file_space_y;
}

void update_selection(Editor& editor, LineBuffer& line_buffer)
{
    zest::Vec2 mouse_pos = zest::zestify(GetMousePosition());

    if (is_inside(mouse_pos, editor.text_area_rect)
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        editor.selecting = true;
        editor.selection_valid = true;
        editor.selection_origin =
            window_to_cursor_pos(editor, line_buffer, mouse_pos);
        editor.selection_current = editor.selection_origin;
    }

    if (editor.selecting)
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            editor.selection_current =
                window_to_cursor_pos(editor, line_buffer, mouse_pos);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            editor.selecting = false;
    }
}


void update(LineBuffer& line_buffer, CursorState& cursor, Editor& editor,
            double time_delta)
{
    update_cursor_direction(line_buffer, cursor, editor, move_cursor_left,
                            KEY_LEFT, cursor.state_left, time_delta);
    update_cursor_direction(line_buffer, cursor, editor, move_cursor_right,
                            KEY_RIGHT, cursor.state_right, time_delta);
    update_cursor_direction(line_buffer, cursor, editor, move_cursor_up,
                            KEY_UP, cursor.state_up, time_delta);
    update_cursor_direction(line_buffer, cursor, editor, move_cursor_down,
                            KEY_DOWN, cursor.state_down, time_delta);

    if (editor.cursorize_view)
        set_file_view_to_cursor(cursor, editor);
    editor.cursorize_view = false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        || IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        set_cursor_to_mouse(cursor, editor, line_buffer);
    }

    cursor.time += time_delta;
    if (cursor.time >= cursor.blink_time)
    {
        cursor.time -= cursor.blink_time;
        cursor.visible = !cursor.visible;
    }

    editor.file_space_y += -GetMouseWheelMove()*editor.cell_height;
    editor.view_rect.y = editor.file_space_y;

    if (editor.file_space_y < 0)
        editor.file_space_y = 0;

    float file_bot = (line_buffer.line_count() - 1) * editor.cell_height;
    if (editor.file_space_y >= file_bot)
        editor.file_space_y = file_bot;

    // Set the mouse cursor to correct image
    zest::Vec2 mouse_pos = zest::zestify(GetMousePosition());
    if (mouse_pos.x >= editor.top_left_x
        && mouse_pos.x <= editor.top_left_x + editor.width
        && mouse_pos.y >= editor.top_left_y
        && mouse_pos.y <= editor.top_left_y + editor.height)
    {
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    update_selection(editor, line_buffer);
}



void draw_clipped_rectangle(Image& img,
                            const zest::Rect& image_rect,
                            const zest::Rect& target_rect,
                            Color color)
{
    zest::Rect rect = zest::clip_rect_to_fit(image_rect, target_rect);

    if (rect.width == 0 || rect.height == 0)
        return;

    zest::raylib::draw_rectangle(img, rect, color);
}

void draw_text_segment(Image* image,
                       const char* text,
                       int from, int to,
                       zest::Vec2 pos,
                       const FontInfo& font_info,
                       std::vector<char>& buffer,
                       Color text_color,
                       std::optional<Color> bg_color)
{
    if (from >= to)
        return;

    int n = to - from;

    if (buffer.size() < n + 1)
        buffer.resize(n + 1);

    std::memcpy(buffer.data(), text + from, n);
    buffer[n] = 0;

    if (bg_color)
        ImageDrawRectangle(image, pos.x, pos.y, n*font_info.char_step,
                           font_info.font_size, *bg_color);

    ImageDrawTextEx(image, font_info.font, buffer.data(), { pos.x, pos.y },
                    font_info.font_size, font_info.char_spacing, text_color);
}


void draw_node(Editor& editor, LineBuffer& line_buff, TSNode node,
               zest::Color color)
{
    TSPoint start = ts_node_start_point(node);
    TSPoint end = ts_node_end_point(node);

    if (end.row != start.row)
    {
        //std::cout << ts_node_type(node) << "\n";
        std::cerr << "PANIC!\n";
        return;
    }

    uint32_t len = end.column - start.column;

    zest::Rect node_rect = {
        start.column*editor.cell_width,
        start.row*editor.cell_height,
        len*editor.cell_width,
        editor.cell_height
    };

    std::vector<char> buffer(len + 1, 0);
    std::memcpy(buffer.data(),
                line_buff.get_line(start.row).c_str() + start.column,
                len);

    if (!zest::are_intersecting(node_rect, editor.view_rect))
        return;

    float x = node_rect.x - editor.view_rect.x;
    float y = node_rect.y - editor.view_rect.y;

    ImageDrawTextEx(&editor.text_area_image,
                    editor.font_info.font,
                    buffer.data(),
                    { x, y },
                    editor.font_info.font_size,
                    editor.font_info.char_spacing,
                    zest::raylib::to_raylib(color));
}

void draw_highlights(Editor& editor, LineBuffer& line_buff)
{
    zest::tree_sitter::TreePtr tree =
        zest::tree_sitter::parse_text(editor.parser.get(), line_buff);

    TSNode root = ts_tree_root_node(tree.get());

    ts_query_cursor_exec(editor.query_cursor.get(),
                         editor.queries.get(),
                         root);

    TSQueryMatch match;
    uint32_t capture_idx;

    while (ts_query_cursor_next_capture(editor.query_cursor.get(),
                                        &match, &capture_idx))
    {
        const TSQueryCapture& capture = match.captures[capture_idx];

        uint32_t name_len;
        const char* capture_name =
            ts_query_capture_name_for_id(editor.queries.get(),
                                         capture.index,
                                         &name_len);
        std::string_view capture_view(capture_name, name_len);

        //std::cout << capture_view << "\n";

        auto it = zest::highlight::highlights.find(capture_view);
        if (it == zest::highlight::highlights.end())
        {
            std::cerr << "Capture not found!\n";
            continue;
        }

        zest::Color color = it->second;
        draw_node(editor, line_buff, capture.node, color);
    }
}

void draw_selection(LineBuffer& line_buffer, CursorState& cursor, Editor& editor)
{
    zest::Rect img_rect = {
        0, 0, editor.text_area_rect.width, editor.text_area_rect.height
    };

    zest::CellPos selection_start = editor.selection_origin;
    zest::CellPos selection_end = editor.selection_current;
    if (editor.selection_origin.line > editor.selection_current.line
        || (editor.selection_origin.line == editor.selection_current.line
            && editor.selection_origin.col >= editor.selection_current.col))
    {
        std::swap(selection_start, selection_end);
    }

    std::vector<char> buffer;

    for (int i = selection_start.line; i <= selection_end.line; ++i)
    {
        int line_len = (int)line_buffer.get_line(i).size();

        int from = i == selection_start.line
                        ? selection_start.col
                        : 0;
        int to = i == selection_end.line
                        ? selection_end.col
                        : line_len;

        int n = to - from;

        float x = from*editor.cell_width - editor.file_space_x;
        float y = i*editor.cell_height - editor.file_space_y;

        int added_len = selection_end.line > selection_start.line
                        && i != selection_end.line
                            ? 1
                            : 0;

        draw_clipped_rectangle(
            editor.text_area_image,
            img_rect,
            { x, y, (n + added_len)*editor.cell_width, editor.cell_height },
            WHITE);

        if (n == 0)
            continue;

        if (buffer.size() < n + 1)
            buffer.resize(n + 1);

        std::memcpy(buffer.data(), &line_buffer.get_line(i)[from], n);
        buffer[n] = 0;

        ImageDrawTextEx(&editor.text_area_image,
                        editor.font_info.font,
                        buffer.data(),
                        { x, y },
                        editor.font_info.font_size,
                        editor.font_info.char_spacing,
                        BLACK);
    }
}

void draw(LineBuffer& line_buffer, CursorState& cursor, Editor& editor)
{
    int font_size = editor.font_info.font_size;

    ImageClearBackground(&editor.text_area_image, BLUE);

    int first_row = editor.file_space_y/font_size;
    int last_row = (editor.file_space_y + editor.height)/font_size;
    int rows = line_buffer.line_count();

    int first_col = editor.file_space_x/editor.cell_width;

    float x = first_col*editor.cell_width - editor.file_space_x;
    float y = first_row*font_size - editor.file_space_y;

    for (int i = first_row; i <= last_row && i < rows; ++i)
    {
        if (first_col < line_buffer.get_line(i).size())
            ImageDrawTextEx(&editor.text_area_image,
                            editor.font_info.font,
                            &line_buffer.get_line(i)[first_col],
                            { x, y },
                            font_size,
                            editor.font_info.char_spacing,
                            WHITE);
        y += font_size;
    }

    if (editor.selection_valid)
        draw_selection(line_buffer, cursor, editor);

    if (cursor.visible)
    {
        float offset_x = cursor.col*editor.cell_width - editor.file_space_x;
        float offset_y = cursor.line*editor.cell_height - editor.file_space_y;

        if (offset_y > -editor.cell_height && offset_y < editor.height)
            draw_clipped_rectangle(
                editor.text_area_image,
                { 0, 0,
                  editor.text_area_rect.width, editor.text_area_rect.height  },
                { offset_x, offset_y,
                  2, (float)font_size },
                WHITE);
    }

    draw_highlights(editor, line_buffer);

    Texture2D text_area_texture = LoadTextureFromImage(editor.text_area_image);

    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(text_area_texture, editor.top_left_x, editor.top_left_y, WHITE);

        DrawRectangleLines(editor.top_left_x - 1, editor.top_left_y - 1,
                           editor.width + 2, editor.height + 2,
                           RED);

        zest::Vec2 mouse_pos = zest::zestify(GetMousePosition());
        DrawRectangle(mouse_pos.x, mouse_pos.y, 2, 2, RED);

    EndDrawing();

    UnloadTexture(text_area_texture);
}


int main(int argc, char** argv)
{
    std::string file_path;
    if (argc < 2)
        file_path = "../main.cpp";
    else
        file_path = argv[1];
    LineBuffer line_buffer = load_file(file_path);

    int fps = 30;
    double target_frame_time = 1.0/60.0;

    int window_width = 800;
    int window_height = 450;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(window_width, window_height, "edwin");

    App app = init_app(window_width, window_height);

    double last_frame_time = 0.0f;
    while (true)
    {
        if (WindowShouldClose())
            break;

        double start_time = GetTime();

        update(line_buffer, app.cursor, app.editor, last_frame_time);
        draw(line_buffer, app.cursor, app.editor);

        double elapsed = GetTime() - start_time;
        if (elapsed < target_frame_time)
            WaitTime(target_frame_time - elapsed);

        last_frame_time = GetTime() - start_time;
    }

    UnloadImage(app.editor.text_area_image);
    UnloadFont(app.editor.font_info.font);

    CloseWindow();
}
