#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

#include "sphere.h"
#include "material.h"
#include "camera.h"
#include "boundingbox.h"


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
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_pos = ImVec2(io.DisplaySize.x - 10.0f, 35.0f); // Offset from top-right
    ImVec2 window_pivot = ImVec2(1.0f, 0.0f);                   // Align top-right corner
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);

    // Allow resizing while staying anchored
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(FLT_MAX, FLT_MAX));

    // Begin the main window
    ImGui::Begin("Octree Manager", nullptr, ImGuiWindowFlags_None);

    // Begin the tab bar
    if (ImGui::BeginTabBar("Octrees")) {

        // Octree Manager tab
        if (ImGui::BeginTabItem("Manager")) {

            auto& octrees = manager.GetOctrees();
            for (size_t i = 0; i < octrees.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));

                // Display the octree name as selectable
                if (ImGui::Selectable(octrees[i].name.c_str(), i == manager.GetSelectedIndex())) {
                    manager.SelectOctree(i);
                }

                ImGui::PopID();
            }

            // Add a button to create a new octree and remove the currently selected one
            ImGui::Separator();
            if (ImGui::Button("Add Octree")) {
                manager.AddOctree("New Octree", BoundingBox(point3(-1, 1, -5), 2.0));
            }
            ImGui::SameLine(); // Place the "Remove Octree" button on the same line
            if (ImGui::Button("Remove Octree")) {
                manager.RemoveSelectedOctree();
            }

            ImGui::EndTabItem();
        }

        // Boolean Operations tab
        if (ImGui::BeginTabItem("Boolean Operations")) {
            RenderBooleanOperations(manager);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void RenderOctreeInspector(OctreeManager& manager, hittable_list& world) {
    static int last_selected_index = -1;
    static char rename_buffer[128];
    static point3 custom_center = { 0.0, 0.0, 0.0 };
    static point3 custom_center_normalized = { 0.0, 0.0, 0.0 };

    int selected_index = manager.GetSelectedIndex();
    if (selected_index < 0 || selected_index >= static_cast<int>(manager.GetOctrees().size())) {
        return;
    }

    OctreeManager::OctreeWrapper& wrapper = manager.GetSelectedOctree();
    BoundingBox& bb = wrapper.octree->bounding_box;
    float width = static_cast<float>(bb.width);
    float sphere_radius_max = static_cast<float>(0.45f * width);
    static float sphere_radius = sphere_radius_max * 0.5f;

    if (selected_index != last_selected_index) {
        last_selected_index = selected_index;
        strncpy(rename_buffer, wrapper.name.c_str(), sizeof(rename_buffer) - 1);
        rename_buffer[sizeof(rename_buffer) - 1] = '\0';

        point3 bb_center = bb.Center();
        custom_center = bb_center;
        custom_center_normalized = { 0.0, 0.0, 0.0 };
    }

    // Octree string representation variables
    static std::string octree_string;
    static bool show_octree_string = false;

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

    ImGui::Begin("Octree Inspector");

    if (ImGui::BeginTabBar("InspectorTabs")) {

        // Inspector Tab
        if (ImGui::BeginTabItem("Inspector")) {
            ImGui::InputText("Octree Name", rename_buffer, sizeof(rename_buffer));
            if (ImGui::Button("Change Name")) {
                manager.SetName(selected_index, std::string(rename_buffer));
            }

            ImGui::Separator();

            point3 corner = bb.vmin;
            float width = static_cast<float>(bb.width);
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

            ImGui::EndTabItem();
        }

        // Primitives Tab
        if (ImGui::BeginTabItem("Primitives")) {
            if (ImGui::BeginTabBar("PrimitiveTabs")) {

                // Sphere Tab
                if (ImGui::BeginTabItem("Sphere")) {
                    static float sphere_radius = sphere_radius_max / 2;
                    static bool centered = true;
                    static bool was_centered = true; // Tracks previous state

                    // Handle the "Centered" checkbox
                    if (ImGui::Checkbox("Centered", &centered)) {
                        if (!centered && was_centered) {
                            // Reset the normalized center when switching from centered to non-centered
                            custom_center_normalized = { 0.0f, 0.0f, 0.0f };
                        }
                        was_centered = centered;
                    }

                    if (!centered) {
                        point3 bb_center = bb.Center();

                        // Use existing normalized values for the drag float
                        float editable_normalized_center[3] = {
                            static_cast<float>(custom_center_normalized.x()),
                            static_cast<float>(custom_center_normalized.y()),
                            static_cast<float>(custom_center_normalized.z())
                        };

                        if (ImGui::DragFloat3("Sphere Center", editable_normalized_center, 0.1f, -1.0f, 1.0f)) {
                            // Update normalized and actual center based on user input
                            custom_center_normalized = point3(editable_normalized_center[0], editable_normalized_center[1], editable_normalized_center[2]);
                            custom_center = point3(
                                bb_center.x() + custom_center_normalized.x() * (bb.width / 2.0),
                                bb_center.y() + custom_center_normalized.y() * (bb.width / 2.0),
                                bb_center.z() + custom_center_normalized.z() * (bb.width / 2.0)
                            );
                        }
                    }

                    ImGui::SliderFloat("Sphere Radius", &sphere_radius, 0.0f, sphere_radius_max);
                    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

                    if (ImGui::Button("Generate From Sphere")) {
                        GenerateOctreeFromSphere(wrapper, bb, sphere_radius, centered, custom_center, manager.depth_limit, octree_string, show_octree_string);
                    }

                    ImGui::EndTabItem();
                }


                // Pyramid Tab (Placeholder)
                if (ImGui::BeginTabItem("Pyramid")) {
                    ImGui::Text("Pyramid generation not implemented yet.");
                    ImGui::EndTabItem();
                }

                // Another Octree Tab
                if (ImGui::BeginTabItem("Another Octree")) {

                    RenderRebuildOctreeUI(manager);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndTabItem();
        }

        // Render Tab
        if (ImGui::BeginTabItem("Render")) {
            static float color_picker[3] = { 1.0f, 1.0f, 1.0f }; // RGB color picker values
            static bool use_random_color = false; // Toggle for random color

            // Color Picker
            ImGui::ColorEdit3("Octree Color", color_picker, ImGuiColorEditFlags_NoInputs);

            // Random Color Checkbox
            ImGui::Checkbox("Random Color", &use_random_color);

            if (ImGui::Button("Render Filled Bounding Boxes")) {
                // Convert RGB color picker values to `double` for material
                mat octree_material(color(static_cast<double>(color_picker[0]),
                    static_cast<double>(color_picker[1]),
                    static_cast<double>(color_picker[2])));

                // Call RenderFilledBBs with the random color flag
                OctreeManager::RenderFilledBBs(manager, selected_index, world, octree_material, use_random_color);
            }

            if (ImGui::Button("Remove Filled Bounding Boxes")) {
                OctreeManager::RemoveFilledBBs(*wrapper.octree, world);
            }

            ImGui::EndTabItem();
        }

        // IO Tab
        if (ImGui::BeginTabItem("IO"))
        {
            ImGui::Text("Octree String Representation:");
            if (show_octree_string) {
                // Display the octree string in a bordered box
                ImGui::BeginChild("OctreeStringBox", ImVec2(ImGui::GetContentRegionAvail().x, 100), true, ImGuiWindowFlags_None);
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("%s", octree_string.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndChild();

                // Button to copy the string to the clipboard
                if (ImGui::Button("Copy to Clipboard")) {
                    ImGui::SetClipboardText(octree_string.c_str());
                }
                ImGui::SameLine(); // Place the next button on the same line
                if (ImGui::Button("Update Representation")) {
                    OctreeManager::OctreeWrapper& wrapper = manager.GetSelectedOctree();
                    octree_string = wrapper.octree->root.ToString();
                    show_octree_string = true;
                }
            }

            ImGui::Separator();

            static std::string input_buffer;
            static std::string error_message;

            ImGui::Text("Input Octree String:");

            ImGuiInputTextFlags flags =
                ImGuiInputTextFlags_CallbackResize;

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::InputText(
                "##OctreeString",
                (char*)input_buffer.c_str(),
                input_buffer.size() + 1,  // +1 for null terminator
                flags,
                InputTextCallback_Resize,
                (void*)&input_buffer
            );

            if (ImGui::Button("Generate Octree From String"))
            {
                try
                {
                    size_t pos = 0;
                    OctreeManager::OctreeWrapper& wrapper = manager.GetSelectedOctree();
                    BoundingBox& bb = wrapper.octree->bounding_box;

                    wrapper.octree = std::make_shared<Octree>(
                        Octree(bb, Node::FromStringRecursive(input_buffer.c_str(), pos))
                    );
                    error_message.clear();
                }
                catch (const std::exception& e)
                {
                    error_message = e.what();
                }
            }

            if (!error_message.empty())
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", error_message.c_str());
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

static int InputTextCallback_Resize(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // "UserData" is a pointer to the std::string being edited.
        std::string* str = reinterpret_cast<std::string*>(data->UserData);

        // Resize the std::string to fit the new text length ImGui wants
        str->resize(data->BufTextLen);

        // Update ImGui buffer pointer to our newly allocated data
        data->Buf = (char*)str->c_str();
    }
    return 0;
}

void GenerateOctreeFromSphere(OctreeManager::OctreeWrapper& wrapper, BoundingBox& bb, float sphere_radius, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string) {
    point3 sphere_center = centered ? bb.Center() : bb.ClosestPoint(custom_center);

    wrapper.octree = std::make_shared<Octree>(
        Octree::FromObject(bb, sphere(sphere_center, static_cast<double>(sphere_radius), mat()), depth_limit)
    );

    // Generate the string representation of the octree
    octree_string = wrapper.octree->root.ToString();
    show_octree_string = true;
}

void RenderBooleanOperations(OctreeManager& manager) {
    // Ensure there are at least two octrees to perform operations
    if (manager.GetOctrees().size() < 2) {
        ImGui::Text("At least two octrees are required for Boolean operations.");
        return;
    }

    // Dropdowns to select the input octrees
    static int octree1_index = 0;
    static int octree2_index = 1;
    static int depth_limit = 3;

    ImGui::Text("Select Octrees for Boolean Operation:");
    ImGui::Combo("Octree 1", &octree1_index, [](void* data, int idx, const char** out_text) {
        auto& octrees = *reinterpret_cast<std::vector<OctreeManager::OctreeWrapper>*>(data);
        *out_text = octrees[idx].name.c_str();
        return true;
        }, &manager.GetOctrees(), static_cast<int>(manager.GetOctrees().size()));

    ImGui::Combo("Octree 2", &octree2_index, [](void* data, int idx, const char** out_text) {
        auto& octrees = *reinterpret_cast<std::vector<OctreeManager::OctreeWrapper>*>(data);
        *out_text = octrees[idx].name.c_str();
        return true;
        }, &manager.GetOctrees(), static_cast<int>(manager.GetOctrees().size()));

    // Depth limit slider
    ImGui::SliderInt("Depth Limit", &depth_limit, 1, 10);

    // Buttons for each Boolean operation
    if (ImGui::Button("Union (OR)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "union", depth_limit);
    }
    ImGui::SameLine();
    if (ImGui::Button("Intersection (AND)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "intersection", depth_limit);
    }
    ImGui::SameLine();
    if (ImGui::Button("Difference (NOT)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "difference", depth_limit);
    }
}

static void RenderRebuildOctreeUI(OctreeManager& manager) {
    // Ensure there are at least two octrees available
    if (manager.GetOctrees().size() < 2) {
        ImGui::Text("At least two octrees are required for rebuilding.");
        return;
    }

    static int selected_octree_index = 0;  // Index of the octree to rebuild
    static int source_octree_index = 1;   // Index of the source octree for bounding box
    static int depth_limit = 3;           // Depth limit for rebuilding

    ImGui::Text("Rebuild Octree from Another:");

    // Dropdown to select the octree to rebuild
    ImGui::Combo("Target Octree", &selected_octree_index, [](void* data, int idx, const char** out_text) {
        auto& octrees = *reinterpret_cast<std::vector<OctreeManager::OctreeWrapper>*>(data);
        *out_text = octrees[idx].name.c_str();
        return true;
        }, &manager.GetOctrees(), static_cast<int>(manager.GetOctrees().size()));

    // Dropdown to select the source octree
    ImGui::Combo("Source Octree", &source_octree_index, [](void* data, int idx, const char** out_text) {
        auto& octrees = *reinterpret_cast<std::vector<OctreeManager::OctreeWrapper>*>(data);
        *out_text = octrees[idx].name.c_str();
        return true;
        }, &manager.GetOctrees(), static_cast<int>(manager.GetOctrees().size()));

    // Slider to set the depth limit
    ImGui::SliderInt("Depth Limit", &depth_limit, 1, 10);

    // Button to trigger the rebuilding process
    if (ImGui::Button("Rebuild Octree")) {
        try {
            // Set the selected octree for rebuilding
            manager.SelectOctree(selected_octree_index);
            // Perform the rebuild operation
            manager.RebuildSelectedOctreeFromAnother(source_octree_index, depth_limit);
            ImGui::Text("Rebuild successful!");
        }
        catch (const std::exception& e) {
            ImGui::Text("Error: %s", e.what());
        }
    }
}
