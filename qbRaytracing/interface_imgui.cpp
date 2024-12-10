#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

#include "sphere.h"
#include "camera.h"

extern point3 random_position();
extern color random_color();

void draw_menu(bool& render_raytracing,
    bool& show_wireframe,
    double& speed,
    Camera& camera,
    point3& origin,
    hittable_list& world) 
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {
        // Primitive Header
        if (ImGui::CollapsingHeader("Primitive")) {
            float speed_f = static_cast<float>(speed);
            ImGui::SliderFloat("Sphere Speed", &speed_f, 1.0f, 50.0f);
            speed = static_cast<double>(speed_f);

            if (ImGui::Button("Spawn Random Sphere")) {
                point3 position = random_position();
                mat material = mat(random_color());
                auto new_sphere = make_shared<sphere>(position, 0.3, material);
                world.add(new_sphere);

                std::cout << "Spawned Random Sphere: \n";
                std::cout << "Position: " << position << "\n";
                std::cout << "Color: " << material.diffuse_color << "\n";
            }
        }
        //// Colors Header
   //if (ImGui::CollapsingHeader("Colors")) {
   //    // Randomize Button
   //    if (ImGui::Button("Randomize Color")) {
   //        red = random_color();
   //        mat new_material(red, 0.8, 1.0, 150.0);
   //        moving_sphere->set_material(new_material);

   //        std::cout << "Moving Sphere Color: " << new_material.diffuse_color << std::endl;
   //    }

   //    ImGui::SameLine();

   //    // Reset Button
   //    if (ImGui::Button("Reset Color")) {
   //        red = color(1.0f, 0.0f, 0.0f); // Default to red
   //        moving_sphere->set_material(mat(vec3(red[0], red[1], red[2]), 0.8, 1.0, 150.0));

   //        std::cout << "Reset Moving Sphere Color to Default Red" << std::endl;
   //    }

   //    // Color Picker for Manual Selection
   //    float color_picker_rgb[3] = { red[0], red[1], red[2] }; // Copy `red`'s current values
   //    if (ImGui::ColorEdit3("Set Sphere Color", color_picker_rgb)) {
   //        // Update `red`'s RGB values and the material
   //        red = color(color_picker_rgb[0], color_picker_rgb[1], color_picker_rgb[2]);
   //        moving_sphere->set_material(mat(vec3(red[0], red[1], red[2]), 0.8, 1.0, 150.0));

   //        std::cout << "Updated Moving Sphere Color via Picker to: "
   //            << "R: " << red[0] << ", G: " << red[1] << ", B: " << red[2] << std::endl;
   //    }
   //}

        // Camera Header
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::Text("Camera Origin");
            float origin_array[3] = { static_cast<float>(camera.origin.x()),
                                      static_cast<float>(camera.origin.y()),
                                      static_cast<float>(camera.origin.z()) };

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("", origin_array, -10.0f, 10.0f)) {
                // Update the camera's origin
                camera.origin = point3(origin_array[0], origin_array[1], origin_array[2]);
                // Recalculate basis vectors to reflect the new origin
                camera.update_basis_vectors();
            }
            ImGui::PopItemWidth();

            // FOV Slider
            float camera_fov_degrees = static_cast<float>(radians_to_degrees(camera.focal_length)); // Convert to degrees
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##", &camera_fov_degrees, 30.0f, 120.0f)) {
                camera.set_focal_length(degrees_to_radians(static_cast<double>(camera_fov_degrees))); // Convert back to radians
            }
            ImGui::PopItemWidth();

            // Reset Button
            if (ImGui::Button("Reset to Default")) {
                // Reset camera origin and FOV to default values
                origin = point3(0, 0, 0);
                camera.origin = origin;
                camera.set_focal_length(degrees_to_radians(60.0)); // Default FOV: 60 degrees
            }
        }

        // Render Header
        if (ImGui::CollapsingHeader("Render")) {
            if (ImGui::Button(render_raytracing ? "Disable Raytracing" : "Enable Raytracing")) {
                render_raytracing = !render_raytracing;
            }
            ImGui::Checkbox("Show Wireframe", &show_wireframe);
        }
    }
    ImGui::End();
}

void draw_fps_counter(float fps) {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 100, 10));
    ImGui::Begin("FPS Counter", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}
