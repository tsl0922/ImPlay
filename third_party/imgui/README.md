# imgui

This is imgui [docking](https://github.com/ocornut/imgui/tree/docking) branch merged with [features/shadows](https://github.com/ocornut/imgui/tree/features/shadows) branch.

#### Included patches

- windows cursor patch (https://github.com/tsl0922/ImPlay/issues/55):
    ```patch
    diff --git a/third_party/imgui/source/imgui_impl_glfw.cpp b/third_party/imgui/source/imgui_impl_glfw.cpp
    index d5d3769..bee3630 100644
    --- a/third_party/imgui/source/imgui_impl_glfw.cpp
    +++ b/third_party/imgui/source/imgui_impl_glfw.cpp
    @@ -790,7 +790,9 @@ static void ImGui_ImplGlfw_UpdateMouseCursor()
            {
                // Show OS mouse cursor
                // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
    +#ifndef _WIN32
                glfwSetCursor(window, bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
    +#endif
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    ```