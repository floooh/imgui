// dear imgui: Platform Binding for emscripten 
// This needs to be used along with a Renderer (usually OpenGL3)
// (Info: this uses emscripten functions for setting up the GL context and input)

//  TODO:
//      - touch input
//      - low-dpi vs high-dpi (currently always high-dpi)
//      - clipboard?
//      - high-dpi (device-pixel-ratio)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include "imgui.h"

#include <stdio.h>

static const char* g_canvas_name;
static double g_canvas_width;
static double g_canvas_height;

// a poor man's input queue to debounce mouse / touch events
static bool g_mouse_down[3];
static bool g_mouse_up[3];

/* callback function which tracks page size changes, and updates canvas size accordingly */
static EM_BOOL css_element_size_changed(int /*event_type*/, const EmscriptenUiEvent* ui_event, void* /*user_data*/) {
    emscripten_get_element_css_size(g_canvas_name, &g_canvas_width, &g_canvas_height);
    /*
        the above function may return a zero size when toggling to HTML5 fullscreen,
        if that happens, use the window's inner size reported by the
        emscripten event.
    */
    if (g_canvas_width < 1.0) {
        g_canvas_width = ui_event->windowInnerWidth;
    }
    if (g_canvas_height < 1.0) {
        g_canvas_height = ui_event->windowInnerHeight;
    }

    // update the WebGL canvas size
    emscripten_set_canvas_element_size(g_canvas_name, g_canvas_width, g_canvas_height);
    return EM_TRUE;
}

bool ImGui_ImplEmsc_Init(const char* canvas_name) {
    IM_ASSERT(canvas_name);
    g_canvas_name = canvas_name;

    // query initial css element size and adjust canvas size
    emscripten_get_element_css_size(g_canvas_name, &g_canvas_width, &g_canvas_height);
    emscripten_set_canvas_element_size(g_canvas_name, g_canvas_width, g_canvas_height);

    // callback for tracking window size changes
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, false, css_element_size_changed);

    // setup the WebGL context (default attributes are fine)
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(g_canvas_name, &attrs);
    if (!ctx) {
        /* no WebGL available */
        return false;
    }
    emscripten_webgl_make_context_current(ctx);
    return true;
}

// input handler callbacks
static EM_BOOL on_keydown(int /*event_type*/, const EmscriptenKeyboardEvent* e, void* /*user_data*/) {
    if (e->keyCode < 512) {
        ImGui::GetIO().KeysDown[e->keyCode] = true;
    }
    // only forward alpha-numeric keys to browser
    return e->keyCode < 32;
}

static EM_BOOL on_keyup(int /*event_type*/, const EmscriptenKeyboardEvent* e, void* /*user_data*/) {
    if (e->keyCode < 512) {
        ImGui::GetIO().KeysDown[e->keyCode] = false;
    }
    // only forward alpha-numeric keys to browser
    return e->keyCode < 32;
}

static EM_BOOL on_keypress(int /*event_type*/, const EmscriptenKeyboardEvent* e, void* /*user_data*/) {
    ImGui::GetIO().AddInputCharacter((ImWchar)e->charCode);
    return true;
}

static EM_BOOL on_mousedown(int /*event_type*/, const EmscriptenMouseEvent* e, void* /*user_data*/) {
    /* middle/right button indices are switched between HTML5 and ImGui */
    switch (e->button) {
        case 0: g_mouse_down[0] = true; break;
        case 1: g_mouse_down[2] = true; break;
        case 2: g_mouse_down[1] = true; break;
    }
    return true;
}

static EM_BOOL on_mouseup(int /*event_type*/, const EmscriptenMouseEvent* e, void* /*user_data*/) {
    /* middle/right button indices are switched between HTML5 and ImGui */
    switch (e->button) {
        case 0: g_mouse_up[0] = true; break;
        case 1: g_mouse_up[2] = true; break;
        case 2: g_mouse_up[1] = true; break;
    }
    return true;
}

static EM_BOOL on_mousemove(int /*event_type*/, const EmscriptenMouseEvent* e, void* /*user_data*/) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos.x = (float) e->canvasX;
    io.MousePos.y = (float) e->canvasY;
    return true;
}

static EM_BOOL on_mousewheel(int /*event_type*/, const EmscriptenWheelEvent* e, void* /*user_data*/) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH = -0.1f * (float)e->deltaX;
    io.MouseWheel = -0.1f * (float)e->deltaY;
    return true;
}

void ImGui_ImplEmsc_InitInput(void) {
    ImGuiIO& io = ImGui::GetIO();

    // emscripten has no constants defined for virtual keycodes
    io.KeyMap[ImGuiKey_Tab] = 9;
    io.KeyMap[ImGuiKey_LeftArrow] = 37;
    io.KeyMap[ImGuiKey_RightArrow] = 39;
    io.KeyMap[ImGuiKey_UpArrow] = 38;
    io.KeyMap[ImGuiKey_DownArrow] = 40;
    io.KeyMap[ImGuiKey_Home] = 36;
    io.KeyMap[ImGuiKey_End] = 35;
    io.KeyMap[ImGuiKey_Delete] = 46;
    io.KeyMap[ImGuiKey_Backspace] = 8;
    io.KeyMap[ImGuiKey_Enter] = 13;
    io.KeyMap[ImGuiKey_Escape] = 27;
    io.KeyMap[ImGuiKey_A] = 65;
    io.KeyMap[ImGuiKey_C] = 67;
    io.KeyMap[ImGuiKey_V] = 86;
    io.KeyMap[ImGuiKey_X] = 88;
    io.KeyMap[ImGuiKey_Y] = 89;
    io.KeyMap[ImGuiKey_Z] = 90;

    // forward emscripten input events to ImGui
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, on_keydown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, on_keyup);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, on_keypress);
    emscripten_set_mousedown_callback(g_canvas_name, nullptr, true, on_mousedown);
    emscripten_set_mouseup_callback(g_canvas_name, nullptr, true, on_mouseup);
    emscripten_set_mousemove_callback(g_canvas_name, nullptr, true, on_mousemove);
    emscripten_set_wheel_callback(g_canvas_name, nullptr, true, on_mousewheel);
    // FIXME: touch events
}

void ImGui_ImplEmsc_NewFrame(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_canvas_width, (float)g_canvas_height);
    io.DeltaTime = 1.0f/60.0f;  // hack, but guarantees smooth animation
    for (int i = 0; i < 3; i++) {
        if (io.MouseDown[i]) {
            if (g_mouse_up[i]) {
                io.MouseDown[i] = false;
                g_mouse_up[i] = false;
            }
        }
        else {
            if (g_mouse_down[i]) {
                io.MouseDown[i] = true;
                g_mouse_down[i] = false;
            }
        }
    }
}