#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

#include "sphere.h"
#include "material.h"
#include "camera.h"


extern point3 random_position();
extern color random_color();

void draw_menu(RenderState& render_state,
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
            float origin_array[3] = {
                static_cast<float>(camera.get_origin().x()),
                static_cast<float>(camera.get_origin().y()),
                static_cast<float>(camera.get_origin().z())
            };

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("Origin", origin_array, -10.0f, 10.0f)) {
                // Update the camera's origin using the setter
                camera.set_origin(point3(origin_array[0], origin_array[1], origin_array[2]));
            }
            ImGui::PopItemWidth();

            // FOV Slider
            float camera_fov_degrees = static_cast<float>(radians_to_degrees(camera.get_focal_length())); // Convert to degrees
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("FOV", &camera_fov_degrees, 30.0f, 120.0f)) {
                // Update the camera's focal length using the setter
                camera.set_focal_length(degrees_to_radians(static_cast<double>(camera_fov_degrees))); // Convert back to radians
            }
            ImGui::PopItemWidth();

            // Reset Button
            if (ImGui::Button("Reset to Default")) {
                // Reset camera origin, FOV, and image width to default values
                camera.set_origin(point3(0, 0, 0));                    // Default origin
                camera.set_focal_length(degrees_to_radians(60.0));     // Default FOV: 60 degrees
                camera.set_image_width(480);                          // Default image width
            }
        }


        // Render Header
        if (ImGui::CollapsingHeader("Render Options")) {

            ImGui::Checkbox("Wireframe", &show_wireframe);

            if (ImGui::Button("High-Res Frame")) {
                render_state.set_mode(HighResolution);
            }

            if (ImGui::Button("Default Render")) {
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
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 90, 5));
    ImGui::Begin("FPS Counter", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}

// Render the list of octrees anchored to the upper-right corner
void RenderOctreeList(OctreeManager& manager) {
    // Anchor the window to the upper-right corner
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_pos = ImVec2(io.DisplaySize.x - 10.0f, 35.0f); // Offset from the top-right corner
    ImVec2 window_pivot = ImVec2(1.0f, 0.0f);                   // Align top-right corner
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);

    // Allow resizing while staying anchored
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(FLT_MAX, FLT_MAX));

    // Begin the window
    ImGui::Begin("Octree Manager", nullptr, ImGuiWindowFlags_None);

    ImGui::Text("Octrees:");
    ImGui::Separator();

    auto& octrees = manager.GetOctrees();
    for (size_t i = 0; i < octrees.size(); ++i) {
        // Generate a unique ID for each octree
        ImGui::PushID(static_cast<int>(i));

        // Display the octree name with a selectable
        if (ImGui::Selectable(octrees[i].name.c_str(), i == manager.GetSelectedIndex())) {
            manager.SelectOctree(i);
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Add Octree")) {
        manager.AddOctree("New Octree", BoundingBox(point3(-1, 1, -5), 2.0));
    }

    // Button to open the Tree Representation window
    static bool show_tree_rep_window = false;
    if (ImGui::Button("Paste Tree Representation")) {
        show_tree_rep_window = true;
    }

    ImGui::End();

    // Render the Tree Representation window if active
    if (show_tree_rep_window) {
        RenderTreeRepresentationWindow(manager, show_tree_rep_window);
    }
}

void RenderOctreeInspector(OctreeManager& manager, hittable_list& world) {
    static int last_selected_index = -1; // Tracks the previously selected index
    static char rename_buffer[128];     // Buffer for renaming input

    int selected_index = manager.GetSelectedIndex();
    if (selected_index < 0 || selected_index >= static_cast<int>(manager.GetOctrees().size())) {
        return;
    }

    // Get the position and size of the "Octree Manager" window
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Octree Manager");
    ImVec2 manager_pos = ImGui::GetWindowPos();
    ImVec2 manager_size = ImGui::GetWindowSize();
    ImGui::End();

    // Anchor the "Octree Inspector" to the bottom of "Octree Manager"
    ImVec2 inspector_pos = ImVec2(manager_pos.x, manager_pos.y + manager_size.y + 10.0f); // 10.0f offset for spacing
    ImGui::SetNextWindowPos(inspector_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(manager_size.x, 300.0f)); // Same width as "Octree Manager" and fixed height

    // Begin the "Octree Inspector" window
    ImGui::Begin("Octree Inspector");

    OctreeManager::OctreeWrapper& wrapper = manager.GetSelectedOctree();

    // If the selected index changes, update the rename buffer
    if (selected_index != last_selected_index) {
        strncpy(rename_buffer, wrapper.name.c_str(), sizeof(rename_buffer) - 1);
        rename_buffer[sizeof(rename_buffer) - 1] = '\0'; // Ensure null termination
        last_selected_index = selected_index;           // Update the last selected index
    }

    // Input text for renaming the octree
    ImGui::InputText("Octree Name", rename_buffer, sizeof(rename_buffer));

    // Button to change the octree's name
    if (ImGui::Button("Change Name")) {
        manager.SetName(selected_index, std::string(rename_buffer));
    }

    ImGui::Separator();

    BoundingBox& bb = wrapper.octree->bounding_box;
    point3 corner = bb.vmin;
    float width = static_cast<float>(bb.width);
    float sphere_radius_max = static_cast<float>(0.45f * width);
    static float sphere_radius = sphere_radius_max * 0.5f;

    float corner_f[3] = { static_cast<float>(corner.x()), static_cast<float>(corner.y()), static_cast<float>(corner.z()) };
    if (ImGui::DragFloat3("Min Corner", corner_f, 0.1f)) {
        bb.set_corner(point3(static_cast<double>(corner_f[0]), static_cast<double>(corner_f[1]), static_cast<double>(corner_f[2])));
    }

    if (ImGui::DragFloat("Width", &width, 0.1f)) {
        bb.set_width(static_cast<double>(width));
        sphere_radius_max = static_cast<float>(0.45 * width);
        sphere_radius = sphere_radius_max * 0.5f; // Clamp sphere radius to new max
    }

    if (ImGui::Button("Reset Octree")) {
        manager.ResetSelectedOctree();
    }

    ImGui::Separator();

    static bool show_octree_string = false;
    static std::string octree_string;

    ImGui::SliderFloat("Sphere Radius", &sphere_radius, 0.0f, sphere_radius_max);
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

    if (ImGui::Button("Generate From Sphere")) {
        wrapper.octree = std::make_shared<Octree>(
            Octree::FromObject(bb, sphere(bb.Center(), static_cast<double>(sphere_radius), mat()), manager.depth_limit)
        );

        // Generate the string representation of the octree
        octree_string = wrapper.octree->root.ToString();
        show_octree_string = true;
    }

    if (show_octree_string) {
        ImGui::Separator();
        ImGui::Text("Octree String Representation:");

        // Enable automatic word wrapping
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x); // Wrap at the current window's width
        ImGui::TextWrapped("%s", octree_string.c_str());          // Display the wrapped text
        ImGui::PopTextWrapPos();                                  // Restore default wrapping behavior

        // Add a button to copy the string to the clipboard
        if (ImGui::Button("Copy to Clipboard")) {
            ImGui::SetClipboardText(octree_string.c_str());
        }
    }

    ImGui::Separator();

    // Add Render and Remove buttons for filled bounding boxes
    if (ImGui::Button("Render Filled Bounding Boxes")) {
        OctreeManager::RenderFilledBBs(manager, selected_index, world);
    }

    if (ImGui::Button("Remove Filled Bounding Boxes")) {
        OctreeManager::RemoveFilledBBs(*wrapper.octree, world);
    }

    ImGui::End();
}

void RenderTreeRepresentationWindow(OctreeManager& manager, bool& show_window) {
    // Get the Octree Inspector window position and size
    ImGui::Begin("Octree Inspector");
    ImVec2 inspector_pos = ImGui::GetWindowPos();
    ImVec2 inspector_size = ImGui::GetWindowSize();
    ImGui::End(); // Close the inspector window to calculate its size correctly

    // Set the position and size for the Tree Representation window
    ImVec2 window_pos = ImVec2(inspector_pos.x, inspector_pos.y + inspector_size.y + 10.0f);
    ImVec2 window_size = ImVec2(inspector_size.x, 150.0f); // Same width as inspector, fixed height
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    // Begin the window
    if (!ImGui::Begin("Tree Representation", &show_window, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    static char input_buffer[1024] = ""; // Buffer for tree representation string
    static std::string error_message;

    ImGui::Text("Paste Tree Representation:");
    ImGui::InputTextMultiline("##TreeRepresentation", input_buffer, sizeof(input_buffer), ImVec2(-FLT_MIN, 60));

    if (ImGui::Button("Generate Octree")) {
        int selected_index = manager.GetSelectedIndex();
        if (selected_index >= 0 && selected_index < static_cast<int>(manager.GetOctrees().size())) {
            try {
                size_t pos = 0;
                auto& wrapper = manager.GetSelectedOctree();
                wrapper.octree = std::make_shared<Octree>(
                    Octree(wrapper.octree->bounding_box, Node::FromStringRecursive(input_buffer, pos))
                );
                error_message.clear(); // Clear error message on success
            }
            catch (const std::exception& e) {
                error_message = e.what(); // Display any parsing error
            }
        }
        else {
            error_message = "No Octree selected.";
        }
    }

    // Display any error messages
    if (!error_message.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", error_message.c_str());
    }

    ImGui::End();
}
