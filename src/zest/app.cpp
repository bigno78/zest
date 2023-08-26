#include "app.hpp"


static int measure_char_width(FontInfo font_info)
{
    Vector2 dims = MeasureTextEx(font_info.font, "A", font_info.font_size,
                                 font_info.char_spacing);
    return dims.x;
}

App init_app(int window_width, int window_height)
{
    App app;

    Editor& editor = app.editor;

    editor.top_left_x = 20;
    editor.top_left_y = 20;
    editor.width = 600;
    editor.height = 400;
    editor.text_area_rect = zest::Rect {
        (float)editor.top_left_x, (float)editor.top_left_y,
        (float)editor.width, (float)editor.height
    };

    editor.file_space_x = 0.0f;
    editor.file_space_y = 0.0f;
    editor.view_rect = zest::Rect {
        editor.file_space_x, editor.file_space_y,
        (float)editor.width, (float)editor.height
    };

    editor.font_info.font_size = 18;
    editor.font_info.font = LoadFontEx("../resources/FiraCode-Regular.ttf",
                                       editor.font_info.font_size,
                                       NULL,
                                       0);
    editor.font_info.char_spacing = 1;
    editor.font_info.char_step = editor.font_info.char_spacing
                        + measure_char_width(editor.font_info);

    editor.cell_width = editor.font_info.char_step;
    editor.cell_height = editor.font_info.font_size;

    editor.text_area_image = GenImageColor(editor.width, editor.height,
                                           { 0, 0, 255, 255 });

    editor.parser = zest::tree_sitter::init();
    editor.queries = zest::tree_sitter::init_highlight_queries(editor.parser.get());
    editor.query_cursor = zest::tree_sitter::init_query_cursor();

    return app;
}
