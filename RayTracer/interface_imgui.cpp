#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

#include "camera.h"

extern point3 random_position();
extern color random_color();

void draw_menu(RenderState& render_state,
    Camera& camera, HittableManager world, std::vector<Light>& lights)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {

        static bool isCameraSpace = false;

        // Camera Header
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::Checkbox("Camera Space Transform", &isCameraSpace);

            // Camera Origin with keyboard control info
            ImGui::Text("Camera Origin (WASD Keys)");  // Modified text
            float origin_array[3] = {
                static_cast<float>(camera.get_origin().x()),
                static_cast<float>(camera.get_origin().y()),
                static_cast<float>(camera.get_origin().z())
            };

            // Fixed width for sliders
            ImGui::PushItemWidth(200.0f); // Set a fixed width for the sliders
            if (ImGui::SliderFloat3("Origin", origin_array, -10.0f, 10.0f)) {
                camera.set_origin(point3(origin_array[0], origin_array[1], origin_array[2]));
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
            ImGui::PopItemWidth();

            // Look At Target with arrow key info
            ImGui::Text("Look At Target (Arrow Keys)");
            float target_array[3] = {
                static_cast<float>(camera.get_look_at().x()),
                static_cast<float>(camera.get_look_at().y()),
                static_cast<float>(camera.get_look_at().z())
            };
            ImGui::PushItemWidth(200.0f); // Same fixed width for consistency
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
            ImGui::PopItemWidth();

            float camera_fov_degrees = static_cast<float>(camera.get_fov_degrees());
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(200.0f); // Same fixed width for consistency
            if (ImGui::SliderFloat("FOV", &camera_fov_degrees, 10.0f, 120.0f)) {
                camera.set_fov(static_cast<double>(camera_fov_degrees));
            }
            ImGui::PopItemWidth();

            if (ImGui::Button("Reset to Default")) {
                camera.set_origin(point3(0, 0, 0));
                camera.set_look_at(point3(0, 0, -3));
                camera.set_fov(60);
                camera.set_image_width(480);
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
        }

        bool renderShadows = camera.shadowStatus();

        // Render Header with keyboard shortcuts
        if (ImGui::CollapsingHeader("Render Options")) {
            ImGui::Text("Press 1-4 to quickly change render mode:");
            ImGui::Separator();

            if (ImGui::Button("Real Time Low-Res Render (1)")) {
                render_state.set_mode(DefaultRender);
            }

            if (ImGui::Button("Low-Res Frame (2)")) {
                render_state.set_mode(LowResolution);
            }

            if (ImGui::Button("High-Res Frame (3)")) {
                render_state.set_mode(HighResolution);
            }

            if (ImGui::Button("Disable Raytracing (4)")) {
                render_state.set_mode(Disabled);
                camera.clear_pixels();
            }

            if (ImGui::Checkbox("Toggle Shadows", &renderShadows)) {
                camera.toggle_shadows();
            }
        }
    }
    ImGui::End();
}

void DrawFpsCounter(float fps) {
    // Set window flags for proper anchoring
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |     // No titlebar, resize handles, etc.
        ImGuiWindowFlags_AlwaysAutoResize | // Auto-fit to content
        ImGuiWindowFlags_NoSavedSettings |  // Don't save position/size
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;            // Prevent user from moving the window

    // Calculate position
    ImVec2 padding = ImVec2(10, 10);  // 10 pixel padding from edges
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // Start the window
    ImGui::SetNextWindowPos(
        ImVec2(displaySize.x - padding.x, displaySize.y - padding.y),
        ImGuiCond_Always,
        ImVec2(1.0f, 1.0f)  // Bottom-right pivot point
    );

    ImGui::Begin("FPS Counter", nullptr, flags);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}