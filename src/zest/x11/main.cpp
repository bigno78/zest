#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include <xcb/xcb.h>

struct XcbWindow
{
    xcb_connection_t* connection;

    int screen_number;
    xcb_screen_t* screen;

    xcb_window_t parent;
    xcb_window_t id;

    uint8_t depth;
    xcb_visualid_t visual_id;

    xcb_gcontext_t gc;
    std::vector<uint32_t> backbuffer;

    int width;
    int height;

    void resize(int new_width, int new_height)
    {
        backbuffer = std::vector<uint32_t>(new_width*new_height, 0);
        width = new_width;
        height = new_height;
    }
};

void print_display_info(XcbWindow& w)
{
    const xcb_setup_t* setup = xcb_get_setup(w.connection);
    std::cout << "vendor: " << xcb_setup_vendor(setup) << "\n";
    std::cout << "image_byte_order: " << int(setup->image_byte_order) << "\n";
    std::cout << "number of screens: " << int(setup->roots_len) << "\n";
    std::cout << "\n";

    xcb_screen_iterator_t screen_iter =
        xcb_setup_roots_iterator(xcb_get_setup(w.connection));

    int i = 0;
    for (; screen_iter.rem; xcb_screen_next(&screen_iter), ++i)
    {
        xcb_screen_t* screen = screen_iter.data;
        std::cout << "screen " << i << "\n";
        std::cout << "  pixels: " << screen->width_in_pixels << "x"
                                  << screen->height_in_pixels << "\n";
        std::cout << "  millis: " << screen->width_in_millimeters << "x"
                                  << screen->height_in_millimeters << "\n";
        std::cout << "  root-depth: " << int(screen->root_depth) << "\n";
        std::cout << "  root-visual: " << int(screen->root_visual) << "\n";
        std::cout << "  backinbg-stores: " << int(screen->backing_stores) << "\n";
        std::cout << "  allowed_depths_len: " << int(screen->allowed_depths_len) << "\n";

        int j = 0;
        xcb_depth_iterator_t depths_iter =
            xcb_screen_allowed_depths_iterator(screen);
        for (; depths_iter.rem; xcb_depth_next (&depths_iter), ++j)
        {
            xcb_depth_t* depth = depths_iter.data;
            //std::cout << "  depth " << int(depth->depth) << "\n";
            //std::cout << "    vusials_len: " << int(depth->visuals_len) << "\n";

            xcb_visualtype_iterator_t visuals_iter =
                xcb_depth_visuals_iterator(depth);
            for (; visuals_iter.rem; xcb_visualtype_next(&visuals_iter))
            {
                xcb_visualtype_t* visual = visuals_iter.data;
                if (visual->visual_id == screen->root_visual)
                {
                    std::cout << "  visual:" << "\n";
                    std::cout << "    bits_per_rgb: " << int(visual->bits_per_rgb_value) << "\n";
                }
            }
        }
    }

    xcb_format_iterator_t format_iter =
        xcb_setup_pixmap_formats_iterator(setup);
    for (; format_iter.rem; xcb_format_next(&format_iter), ++i)
    {
        xcb_format_t* format = format_iter.data;
        std::cout << "  Format for depth " << int(format->depth) << "\n";
        std::cout << "    bits per pixel: " << int(format->bits_per_pixel) << "\n";
        std::cout << "    scanline pad: " << int(format->scanline_pad) << "\n";
    }
}

void print_event(xcb_generic_event_t* event)
{
    if (!event)
    {
        std::cout << "event is NULL\n";
        return;
    }

    uint8_t msb = 0x1 << 7;
    bool is_weird = msb & event->response_type;
    uint8_t code = event->response_type;
    if (is_weird)
    {
        std::cout << "Weird:\n";
        code -= msb;
    }

    switch (code)
    {
        case XCB_CREATE_NOTIFY:
        {
            std::cout << "Create notify\n";
            break;
        }
        case XCB_REPARENT_NOTIFY:
        {
            std::cout << "Reparent notify\n";
            break;
        }
        case XCB_PROPERTY_NOTIFY:
        {
            std::cout << "Property notify\n";
            break;
        }
        case XCB_CONFIGURE_NOTIFY:
        {
            std::cout << "Configure notify\n";
            break;
        }
        case XCB_MAP_NOTIFY:
        {
            std::cout << "Map notify\n";
            break;
        }
        case XCB_VISIBILITY_NOTIFY:
        {
            std::cout << "Visibility notify\n";
            break;
        }
        case XCB_EXPOSE:
        {
            std::cout << "Expose\n";
            break;
        }
        case XCB_FOCUS_IN:
        {
            std::cout << "Focus in\n";
            break;
        }
        case XCB_KEYMAP_NOTIFY:
        {
            std::cout << "Keymap notify\n";
            break;
        }
        default:
        {
            std::cout << "IDK: " << int(event->response_type) << "\n";
            break;
        }
    }
}

int main ()
{
    XcbWindow w;

    w.connection = xcb_connect(NULL, &w.screen_number);

    if (!w.connection)
    {
        std::cerr << "Failed to establish connection.\n";
        return 1;
    }

    // print_display_info(w);

    const xcb_setup_t* setup = xcb_get_setup(w.connection);

    w.screen = nullptr;
    xcb_screen_iterator_t screen_iter =
        xcb_setup_roots_iterator(xcb_get_setup(w.connection));
    for (int i = 0; screen_iter.rem; xcb_screen_next(&screen_iter), ++i)
    {
        if (i == w.screen_number)
        {
            w.screen = screen_iter.data;
            break;
        }
    }

    if (!w.screen)
    {
        std::cerr << "Screen " << w.screen_number << " not found.\n";
        return 1;
    }

    w.parent = w.screen->root;
    w.id = xcb_generate_id(w.connection);
    w.depth = w.screen->root_depth;
    w.visual_id = w.screen->root_visual;
    w.width = 800;
    w.height = 600;

    // mask of included window attributes
    uint32_t value_mask = XCB_CW_EVENT_MASK;
    // fill in the attribute values
    xcb_create_window_value_list_t value_list;
    value_list.event_mask = 0x01ffffff;
    // serialize them
    void* value_list_buffer = NULL;
    xcb_create_window_value_list_serialize(&value_list_buffer, value_mask, &value_list);

    xcb_void_cookie_t win_cookie =
        xcb_create_window_checked(w.connection,
                                  w.depth,
                                  w.id,
                                  w.parent,
                                  200, 100, // x, y
                                  w.width, w.height,
                                  2, // border width
                                  XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                  w.visual_id,
                                  value_mask,
                                  value_list_buffer);
    xcb_void_cookie_t map_cookie = xcb_map_window(w.connection, w.id);
    xcb_flush(w.connection);

    xcb_generic_error_t* err = xcb_request_check(w.connection, win_cookie);
    if (err)
    {
        std::cout << "error: create_window:" << err->error_code << "\n";
        return 1;
    }

    err = xcb_request_check(w.connection, map_cookie);
    if (err)
    {
        std::cout << "error: map_window:" << err->error_code << "\n";
        return 1;
    }

    std::vector<uint32_t> buffer(w.width*w.height);
    for (int y = 0; y < w.height; ++y)
    {
        for (int x = 0; x < 200; ++x)
        {
            buffer[y*w.width + x] = 0x000000ff;
        }
    }
    for (int y = 0; y < w.height; ++y)
    {
        for (int x = 200; x < w.width; ++x)
        {
            buffer[y*w.width + x] = 0x0000FF00;
        }
    }

    w.gc = xcb_generate_id(w.connection);
    xcb_void_cookie_t cookie = xcb_create_gc_checked (w.connection,
                            w.gc,
                            w.id,
                            0,
                            NULL);
    xcb_flush (w.connection);
    err = xcb_request_check (w.connection, cookie);
    if (err)
    {
        std::cout << "error: create_gc\n";
    }

    cookie = xcb_put_image_checked (w.connection,
                   XCB_IMAGE_FORMAT_Z_PIXMAP,
                   w.id,
                   w.gc,
                   w.width,
                   w.height,
                   0,
                   0,
                   0,
                   w.depth,
                   buffer.size()*4,
                   (const uint8_t*)buffer.data());
    xcb_flush (w.connection);
    err = xcb_request_check (w.connection, cookie);
    if (err)
    {
        std::cout << "error: put_image\n";
    }

    xcb_generic_event_t* event;// = xcb_wait_for_event(w.connection);
    //print_event(event);
    while (true)
    {
        event = xcb_poll_for_event(w.connection);
        if (event)
            print_event(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
