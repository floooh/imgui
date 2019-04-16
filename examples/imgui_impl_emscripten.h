// dear imgui: Platform Binding for emscripten 
// This needs to be used along with a Renderer (usually OpenGL3)
// (Info: this uses emscripten functions for setting up the GL context and input)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once

// setup WebGL context, usually use "#canvas" for canvas name
IMGUI_IMPL_API bool ImGui_ImplEmsc_Init(const char* canvas_name);
// call after intializing ImGui, this will setup the input forwarding
IMGUI_IMPL_API void ImGui_ImplEmsc_InitInput(void);
// call before ImGui::NewFrame()
IMGUI_IMPL_API void ImGui_ImplEmsc_NewFrame(void);
