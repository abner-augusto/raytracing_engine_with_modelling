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
    ImGui::SetNextWindowSize(ImVec2(250, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {

        static bool isCameraSpace = false; // Checkbox state

        // Camera Header
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::Checkbox("Camera Space Transform", &isCameraSpace);

            ImGui::Text("Camera Origin");
            float origin_array[3] = {
                static_cast<float>(camera.get_origin().x()),
                static_cast<float>(camera.get_origin().y()),
                static_cast<float>(camera.get_origin().z())
            };

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("Origin", origin_array, -10.0f, 10.0f)) {
                // Update the camera's origin and recalculate matrices
                camera.set_origin(point3(origin_array[0], origin_array[1], origin_array[2]));
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
            ImGui::PopItemWidth();

            // Look At Target Slider
            ImGui::Text("Look At Target");
            float target_array[3] = {
                static_cast<float>(camera.get_look_at().x()),
                static_cast<float>(camera.get_look_at().y()),
                static_cast<float>(camera.get_look_at().z())
            };
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                // Update the camera's look-at point and recalculate matrices
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
            ImGui::PopItemWidth();

            // FOV Slider
            float camera_fov_degrees = static_cast<float>(camera.get_fov_degrees()); // Get FOV in degrees
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("FOV", &camera_fov_degrees, 10.0f, 120.0f)) {
                // Update the camera's FOV using the setter
                camera.set_fov(static_cast<double>(camera_fov_degrees)); // Set FOV in degrees
            }
            ImGui::PopItemWidth();

            // Reset Button
            if (ImGui::Button("Reset to Default")) {
                // Reset camera origin, FOV, and image width to default values
                camera.set_origin(point3(0, 0, 0));
                camera.set_look_at(point3(0, 0, -3));
                camera.set_fov(60);
                camera.set_image_width(480);
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
        }

        // Render Header
        if (ImGui::CollapsingHeader("Render Options")) {

            if (ImGui::Button("High-Res Frame")) {
                render_state.set_mode(HighResolution);
            }

            if (ImGui::Button("Low-Res Frame")) {
                render_state.set_mode(LowResolution);
            }

            if (ImGui::Button("Real Time Low-Res Render")) {
                render_state.set_mode(DefaultRender);
            }

            if (ImGui::Button("Disable Raytracing")) {
                render_state.set_mode(Disabled);
                camera.clear_pixels();
            }
        }
    }
    ImGui::End();
}

void DrawFpsCounter(float fps) {
    // Calculate the height of the FPS counter window
    ImVec2 windowSize = ImVec2(90, 10);
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // Set the next window position to the bottom right
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - windowSize.x - 10, displaySize.y - windowSize.y - 10)); // 10 pixels from the bottom and right
    ImGui::Begin("FPS Counter", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}