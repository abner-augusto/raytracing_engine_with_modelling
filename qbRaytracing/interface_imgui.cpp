#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

// Include any headers needed for your scene objects and helper functions
#include "sphere.h"
#include "plane.h"
#include "cylinder.h"
#include "box.h"
#include "boundingbox.h"
#include "node.h"

// External functions if you have them defined elsewhere
extern point3 random_position();
extern color random_color();

void draw_menu(bool& render_raytracing,
    bool& show_wireframe,
    double& speed,
    double& camera_fov,
    float camera_origin[3],
    double aspect_ratio,
    point3& origin,
    vec3& horizontal,
    vec3& vertical,
    vec3& lower_left_corner,
    double& focal_length,
    double& viewport_width,
    double& viewport_height,
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
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("", camera_origin, -10.0f, 10.0f)) {
                origin = point3(camera_origin[0], camera_origin[1], -camera_origin[2]);
                double viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2);
                double viewport_width = aspect_ratio * viewport_height;
                horizontal = vec3(viewport_width, 0, 0);
                vertical = vec3(0, viewport_height, 0);
                lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
            }

            float camera_fov_f = static_cast<float>(camera_fov);
            ImGui::PopItemWidth();
            // Slider para ajustar o campo de visão (FOV)
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##", &camera_fov_f, 30.0f, 120.0f)) {
                // Convert back to double
                camera_fov = static_cast<double>(camera_fov_f);

                viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2.0);
                viewport_width = aspect_ratio * viewport_height;

                // Update focal length to match the new FOV
                focal_length = degrees_to_radians(camera_fov);

                horizontal = vec3(viewport_width, 0, 0);
                vertical = vec3(0, viewport_height, 0);
                lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 - vec3(0, 0, focal_length);
            }
            ImGui::PopItemWidth();

            if (ImGui::Button("Reset to Default")) {
                // Restaurar os valores padrão
                camera_origin[0] = 0;
                camera_origin[1] = 0;
                camera_origin[2] = 0;
                camera_fov = 60.0f;
                // Recalcular parâmetros da câmera
                origin = point3(camera_origin[0], camera_origin[1], camera_origin[2]);
                double viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2);
                double viewport_width = aspect_ratio * viewport_height;
                horizontal = vec3(viewport_width, 0, 0);
                vertical = vec3(0, viewport_height, 0);
                lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
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
