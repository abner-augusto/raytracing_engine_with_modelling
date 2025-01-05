#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"
#include <imgui.h>
#include <cmath>

#include "sphere.h"
#include "squarepyramid.h"
#include "cylinder.h"
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
        /**if (ImGui::CollapsingHeader("Primitive")) {
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
        }**/

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

            // Look At Target Slider
            ImGui::Text("Look At Target");
            float target_array[3] = {
                static_cast<float>(camera.get_look_at().x()),
                static_cast<float>(camera.get_look_at().y()),
                static_cast<float>(camera.get_look_at().z())
            };
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                // Update the camera's look at target using the setter
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
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
            }
        }


        // Render Header
        if (ImGui::CollapsingHeader("Render Options")) {

            ImGui::Checkbox("Wireframe", &show_wireframe);

            if (ImGui::Button("High-Res Frame")) {
                render_state.set_mode(HighResolution);
            }

            if (ImGui::Button("Low-Res Frame")) {
                render_state.set_mode(LowResolution);
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
                manager.AddOctree("New Octree", BoundingBox(point3(-1, -0.5, -5), 2.0));
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

struct OctreeInspectorState
{
    int   last_selected_index = -1;
    char  rename_buffer[128] = {};

    point3 custom_center = { 0.0, 0.0, 0.0 };
    point3 custom_center_normalized = { 0.0, 0.0, 0.0 };

    std::string octree_string;
    bool        show_octree_string = false;

    std::string input_buffer;
    std::string error_message;

    // For sphere generation
    float sphere_radius = 0.0f;
    float sphere_radius_max = 0.0f;
    bool  centered = true;
    bool  was_centered = true; // Tracks previous state for "centered"

    // For color
    float color_picker[3] = { 1.0f, 1.0f, 1.0f };
    bool  use_random_color = false;

    // For pyramid generation
    float pyramid_height = 0.0f;
    float pyramid_height_max = 0.0f;
    float pyramid_base = 0.0f;
    float pyramid_base_max = 0.0f;

    // For cylinder generation
    float cylinder_height = 0.0f;
    float cylinder_height_max = 0.0f;
    float cylinder_radius = 0.0f;
    float cylinder_radius_max = 0.0f;

    // For cube generation
    float cube_width = 0.0f;
    float cube_width_max = 0.0f;

    // For volume and voxels
    double displayed_volume = 0.0;
    size_t displayed_voxels = 0;
    bool volume_voxels_updated = false;
};


static OctreeInspectorState s_state;

void RenderOctreeInspector(OctreeManager& manager, hittable_list& world)
{
    // Early out if nothing is selected
    int selected_index = manager.GetSelectedIndex();
    if (selected_index < 0 || selected_index >= static_cast<int>(manager.GetOctrees().size()))
    {
        return;
    }

    // Grab the selected octree wrapper & bounding box
    OctreeManager::OctreeWrapper& wrapper = manager.GetSelectedOctree();
    BoundingBox& bb = wrapper.octree->bounding_box;

    // If user selected a new octree, reset state variables
    if (selected_index != s_state.last_selected_index)
    {
        s_state.last_selected_index = selected_index;
        strncpy(s_state.rename_buffer, wrapper.name.c_str(), sizeof(s_state.rename_buffer) - 1);
        s_state.rename_buffer[sizeof(s_state.rename_buffer) - 1] = '\0';

        point3 bb_center = bb.Center();
        s_state.custom_center = bb_center;
        s_state.custom_center_normalized = { 0.0, 0.0, 0.0 };

        // Recompute dimension limits based on bounding box width
        float width = static_cast<float>(bb.width);

        // SPHERE
        s_state.sphere_radius_max = 0.5f * width;
        s_state.sphere_radius = s_state.sphere_radius_max * 0.5f;
        // PYRAMID
        s_state.pyramid_height_max = 0.9f * width;
        s_state.pyramid_height = s_state.pyramid_height_max * 0.5f;

        s_state.pyramid_base_max = 0.9f * width;
        s_state.pyramid_base = s_state.pyramid_base_max * 0.5f;
        // CYLINDER
        s_state.cylinder_height_max = 0.9f * width;
        s_state.cylinder_height = s_state.cylinder_height_max * 0.5f;

        s_state.cylinder_radius_max = 0.5f * width;
        s_state.cylinder_radius = s_state.cylinder_radius_max * 0.5f;
        // CUBE
        s_state.cube_width_max = 0.9f * width;
        s_state.cube_width = s_state.cube_width_max * 0.5f;
    }

    // Get "Octree Manager" window pos/size so we can anchor below it
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Octree Manager");
    ImVec2 manager_pos = ImGui::GetWindowPos();
    ImVec2 manager_size = ImGui::GetWindowSize();
    ImGui::End();

    // Position/size the Inspector window under "Octree Manager"
    ImVec2 inspector_pos = ImVec2(manager_pos.x, manager_pos.y + manager_size.y + 10.0f);
    ImGui::SetNextWindowPos(inspector_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(manager_size.x, 300.0f));

    ImGui::Begin("Octree Inspector");
    if (ImGui::BeginTabBar("InspectorTabs"))
    {
        // Inspector Tab
        if (ImGui::BeginTabItem("Inspector"))
        {
            RenderInspectorTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Primitives Tab
        if (ImGui::BeginTabItem("Primitives"))
        {
            RenderPrimitivesTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Render Tab
        if (ImGui::BeginTabItem("Render"))
        {
            RenderRenderTab(manager, selected_index, world, wrapper);
            ImGui::EndTabItem();
        }

        // IO Tab
        if (ImGui::BeginTabItem("IO"))
        {
            RenderIOTab(manager, selected_index);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}


// Helper for the "Inspector" tab
static void RenderInspectorTab(OctreeManager& manager, int selected_index, BoundingBox& bb) {
    const auto& octrees = manager.GetOctrees();

    // Use the provided `selected_index` parameter directly
    if (selected_index < 0 || selected_index >= static_cast<int>(octrees.size())) {
        ImGui::Text("No octree selected.");
        return;
    }

    auto& selected_octree = octrees[selected_index];
    // Use the `bb` parameter directly
    bb = selected_octree.octree->bounding_box;

    // Display the name and allow renaming
    ImGui::InputText("Octree Name", s_state.rename_buffer, sizeof(s_state.rename_buffer));
    if (ImGui::Button("Change Name")) {
        manager.SetName(selected_index, std::string(s_state.rename_buffer));
    }

    ImGui::Separator();

    // Display and allow modifying the bounding box corner and width
    point3 corner = bb.vmin;
    float width = static_cast<float>(bb.width);

    float corner_f[3] = {
        static_cast<float>(corner.x()),
        static_cast<float>(corner.y()),
        static_cast<float>(corner.z())
    };

    if (ImGui::DragFloat3("Min Corner", corner_f, 0.1f)) {
        bb.set_corner(point3(
            static_cast<double>(corner_f[0]),
            static_cast<double>(corner_f[1]),
            static_cast<double>(corner_f[2])
        ));
    }

    if (ImGui::DragFloat("Width", &width, 0.1f)) {
        bb.set_width(static_cast<double>(width));
    }

    if (ImGui::Button("Reset Octree")) {
        manager.ResetSelectedOctree();
    }

    ImGui::Separator();

    // Display volume and total voxels of the selected octree
    if (ImGui::Button("Calculate Volume")) {
        try {
            s_state.displayed_volume = manager.ComputeSelectedOctreeVolume();
            s_state.displayed_voxels = selected_octree.octree->GetFilledBoundingBoxes().size();
            s_state.volume_voxels_updated = true;
        }
        catch (const std::exception& e) {
            ImGui::Text("Error: %s", e.what());
        }
    }

    if (s_state.volume_voxels_updated) {
        ImGui::Text("Volume: %.2f m3", s_state.displayed_volume);
        ImGui::Text("Total Voxels: %zu", s_state.displayed_voxels);
    }

    if (ImGui::Button("Print Octree")) {
        manager.PrintOctreeHierarchy(selected_index);
    }
}


// Helper for the main "Primitives" tab
static void RenderPrimitivesTab(OctreeManager& manager, int selected_index, BoundingBox& bb)
{
    if (ImGui::BeginTabBar("PrimitiveTabs"))
    {
        // Cube Tab
        if (ImGui::BeginTabItem("Cube"))
        {
            RenderPrimitivesCubeTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Sphere Tab
        if (ImGui::BeginTabItem("Sphere"))
        {
            RenderPrimitivesSphereTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Pyramid Tab
        if (ImGui::BeginTabItem("Pyramid"))
        {
            RenderPrimitivesPyramidTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Cylinder Tab
        if (ImGui::BeginTabItem("Cylinder"))
        {
            RenderPrimitivesCylinderTab(manager, selected_index, bb);
            ImGui::EndTabItem();
        }

        // Another Octree Tab
        if (ImGui::BeginTabItem("Another Octree"))
        {
            RenderRebuildOctreeUI(manager);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

static void RenderPrimitivesCubeTab(OctreeManager& manager, int selected_index, BoundingBox& bb)
{
    // Checkbox for centered
    if (ImGui::Checkbox("Centered", &s_state.centered))
    {
        if (!s_state.centered && s_state.was_centered)
        {
            s_state.custom_center_normalized = { 0.0f, 0.0f, 0.0f };
        }
        s_state.was_centered = s_state.centered;
    }

    // If not centered, let user drag the center (normalized)
    if (!s_state.centered)
    {
        point3 bb_center = bb.Center();
        float editable_normalized_center[3] = {
            static_cast<float>(s_state.custom_center_normalized.x()),
            static_cast<float>(s_state.custom_center_normalized.y()),
            static_cast<float>(s_state.custom_center_normalized.z())
        };

        if (ImGui::DragFloat3("Cube Center", editable_normalized_center, 0.1f, -1.0f, 1.0f))
        {
            s_state.custom_center_normalized = point3(
                editable_normalized_center[0],
                editable_normalized_center[1],
                editable_normalized_center[2]
            );
            s_state.custom_center = point3(
                bb_center.x() + s_state.custom_center_normalized.x() * (bb.width / 2.0),
                bb_center.y() + s_state.custom_center_normalized.y() * (bb.width / 2.0),
                bb_center.z() + s_state.custom_center_normalized.z() * (bb.width / 2.0)
            );
        }
    }

    // Slider for cube dimension
    ImGui::SliderFloat("Cube Width", &s_state.cube_width, 0.0f, s_state.cube_width_max);
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

    // Generate button
    if (ImGui::Button("Generate From Cube"))
    {
        GenerateOctreeFromBox(
            manager,
            selected_index,
            bb,
            s_state.cube_width,
            s_state.centered,
            s_state.custom_center,
            manager.depth_limit,
            s_state.octree_string,
            s_state.show_octree_string
        );
    }
}

// Helper for the "Sphere" sub-tab inside "Primitives"
static void RenderPrimitivesSphereTab(OctreeManager& manager, int selected_index, BoundingBox& bb)
{
    // Handle the "Centered" checkbox
    if (ImGui::Checkbox("Centered", &s_state.centered))
    {
        // If switching from centered -> non-centered,
        // reset the normalized center to 0
        if (!s_state.centered && s_state.was_centered)
        {
            s_state.custom_center_normalized = { 0.0f, 0.0f, 0.0f };
        }
        s_state.was_centered = s_state.centered;
    }

    // If not centered, allow user to drag sphere center
    if (!s_state.centered)
    {
        point3 bb_center = bb.Center();
        float editable_normalized_center[3] = {
            static_cast<float>(s_state.custom_center_normalized.x()),
            static_cast<float>(s_state.custom_center_normalized.y()),
            static_cast<float>(s_state.custom_center_normalized.z())
        };

        if (ImGui::DragFloat3("Sphere Center", editable_normalized_center, 0.1f, -1.0f, 1.0f))
        {
            s_state.custom_center_normalized = point3(
                editable_normalized_center[0],
                editable_normalized_center[1],
                editable_normalized_center[2]
            );
            s_state.custom_center = point3(
                bb_center.x() + s_state.custom_center_normalized.x() * (bb.width / 2.0),
                bb_center.y() + s_state.custom_center_normalized.y() * (bb.width / 2.0),
                bb_center.z() + s_state.custom_center_normalized.z() * (bb.width / 2.0)
            );
        }
    }

    ImGui::SliderFloat("Sphere Radius", &s_state.sphere_radius, 0.0f, s_state.sphere_radius_max);
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

    if (ImGui::Button("Generate From Sphere"))
    {
        GenerateOctreeFromSphere(
            manager,
            selected_index,
            bb,
            s_state.sphere_radius,
            s_state.centered,
            s_state.custom_center,
            manager.depth_limit,
            s_state.octree_string,
            s_state.show_octree_string
        );
    }
}

static void RenderPrimitivesPyramidTab(OctreeManager& manager, int selected_index, BoundingBox& bb)
{
    // Checkbox for centered
    if (ImGui::Checkbox("Centered", &s_state.centered))
    {
        // If switching from centered -> non-centered, reset the normalized center
        if (!s_state.centered && s_state.was_centered)
        {
            s_state.custom_center_normalized = { 0.0f, 0.0f, 0.0f };
        }
        s_state.was_centered = s_state.centered;
    }

    // If not centered, let user drag the center (normalized)
    if (!s_state.centered)
    {
        point3 bb_center = bb.Center();
        float editable_normalized_center[3] = {
            static_cast<float>(s_state.custom_center_normalized.x()),
            static_cast<float>(s_state.custom_center_normalized.y()),
            static_cast<float>(s_state.custom_center_normalized.z())
        };

        // DragFloat3 for pyramid center
        if (ImGui::DragFloat3("Pyramid Center", editable_normalized_center, 0.1f, -1.0f, 1.0f))
        {
            s_state.custom_center_normalized = point3(
                editable_normalized_center[0],
                editable_normalized_center[1],
                editable_normalized_center[2]
            );
            s_state.custom_center = point3(
                bb_center.x() + s_state.custom_center_normalized.x() * (bb.width / 2.0),
                bb_center.y() + s_state.custom_center_normalized.y() * (bb.width / 2.0),
                bb_center.z() + s_state.custom_center_normalized.z() * (bb.width / 2.0)
            );
        }
    }

    // Sliders for pyramid size
    ImGui::SliderFloat("Pyramid Height", &s_state.pyramid_height, 0.0f, s_state.pyramid_height_max);
    ImGui::SliderFloat("Pyramid Base", &s_state.pyramid_base, 0.0f, s_state.pyramid_base_max);
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

    // Generate button
    if (ImGui::Button("Generate From Pyramid"))
    {
        GenerateOctreeFromSquarePyramid(
            manager,
            selected_index,
            bb,
            s_state.pyramid_height,
            s_state.pyramid_base,
            s_state.centered,
            s_state.custom_center,
            manager.depth_limit,
            s_state.octree_string,
            s_state.show_octree_string
        );
    }
}

static void RenderPrimitivesCylinderTab(OctreeManager& manager, int selected_index, BoundingBox& bb)
{
    // Checkbox for centered
    if (ImGui::Checkbox("Centered", &s_state.centered))
    {
        if (!s_state.centered && s_state.was_centered)
        {
            s_state.custom_center_normalized = { 0.0f, 0.0f, 0.0f };
        }
        s_state.was_centered = s_state.centered;
    }

    // If not centered, let user drag the center (normalized)
    if (!s_state.centered)
    {
        point3 bb_center = bb.Center();
        float editable_normalized_center[3] = {
            static_cast<float>(s_state.custom_center_normalized.x()),
            static_cast<float>(s_state.custom_center_normalized.y()),
            static_cast<float>(s_state.custom_center_normalized.z())
        };

        if (ImGui::DragFloat3("Cylinder Center", editable_normalized_center, 0.1f, -1.0f, 1.0f))
        {
            s_state.custom_center_normalized = point3(
                editable_normalized_center[0],
                editable_normalized_center[1],
                editable_normalized_center[2]
            );
            s_state.custom_center = point3(
                bb_center.x() + s_state.custom_center_normalized.x() * (bb.width / 2.0),
                bb_center.y() + s_state.custom_center_normalized.y() * (bb.width / 2.0),
                bb_center.z() + s_state.custom_center_normalized.z() * (bb.width / 2.0)
            );
        }
    }

    // Sliders for cylinder dimensions
    ImGui::SliderFloat("Cylinder Height", &s_state.cylinder_height, 0.0f, s_state.cylinder_height_max);
    ImGui::SliderFloat("Cylinder Radius", &s_state.cylinder_radius, 0.0f, s_state.cylinder_radius_max);
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 5);

    // Generate button
    if (ImGui::Button("Generate From Cylinder"))
    {
        GenerateOctreeFromCylinder(
            manager,
            selected_index,
            bb,
            static_cast<double>(s_state.cylinder_height),
            static_cast<double>(s_state.cylinder_radius),
            s_state.centered,
            s_state.custom_center,
            manager.depth_limit,
            s_state.octree_string,
            s_state.show_octree_string
        );
    }
}

// Helper for the "Render" tab
static void RenderRenderTab(OctreeManager& manager, int selected_index, hittable_list& world, OctreeManager::OctreeWrapper& wrapper)
{
    // Color Picker
    ImGui::ColorEdit3("Octree Color", s_state.color_picker, ImGuiColorEditFlags_NoInputs);

    // Random Color Toggle
    ImGui::Checkbox("Random Color", &s_state.use_random_color);

    if (ImGui::Button("Render Filled Bounding Boxes"))
    {
        mat octree_material(
            color(
                static_cast<double>(s_state.color_picker[0]),
                static_cast<double>(s_state.color_picker[1]),
                static_cast<double>(s_state.color_picker[2])
            )
        );

        // Call RenderFilledBBs with the random color flag
        OctreeManager::RenderFilledBBs(manager, selected_index, world, octree_material, s_state.use_random_color);
    }

    if (ImGui::Button("Remove Filled Bounding Boxes"))
    {
        OctreeManager::RemoveFilledBBs(*wrapper.octree, world);
    }
}

// Helper for the "IO" tab
static void RenderIOTab(OctreeManager& manager, int selected_index)
{
    ImGui::Text("Octree String Representation:");
    if (s_state.show_octree_string)
    {
        ImGui::BeginChild("OctreeStringBox", ImVec2(ImGui::GetContentRegionAvail().x, 100), true);
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextWrapped("%s", s_state.octree_string.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndChild();

        // Copy to clipboard
        if (ImGui::Button("Copy to Clipboard"))
        {
            ImGui::SetClipboardText(s_state.octree_string.c_str());
        }
        ImGui::SameLine();

        // Update representation
        if (ImGui::Button("Update Representation"))
        {
            // Just re-pull the string from the current octree
            auto& wrapper = manager.GetSelectedOctree();
            s_state.octree_string = wrapper.octree->root.ToString();
            s_state.show_octree_string = true;
        }
    }

    ImGui::Separator();
    ImGui::Text("Input Octree String:");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    // Input text for user string
    ImGui::InputText(
        "##OctreeString",
        (char*)s_state.input_buffer.c_str(),
        s_state.input_buffer.size() + 1,  // +1 for null terminator
        flags,
        InputTextCallback_Resize,
        (void*)&s_state.input_buffer
    );

    // Generate from string button
    if (ImGui::Button("Generate Octree From String"))
    {
        try
        {
            size_t pos = 0;
            auto& wrapper = manager.GetSelectedOctree();
            auto& bb = wrapper.octree->bounding_box;

            wrapper.octree = std::make_shared<Octree>(
                Octree(bb, Node::FromStringRecursive(s_state.input_buffer.c_str(), pos))
            );
            s_state.error_message.clear();
        }
        catch (const std::exception& e)
        {
            s_state.error_message = e.what();
        }
    }

    // Show error (if any)
    if (!s_state.error_message.empty())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", s_state.error_message.c_str());
    }
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
static void GenerateOctreeFromSphere(
    OctreeManager& manager,
    int selected_index,
    BoundingBox& bb,
    float sphere_radius,
    bool centered,
    point3 custom_center,
    int depth_limit,
    std::string& octree_string,
    bool& show_octree_string
) {
    // Compute the sphere's center
    point3 sphere_center = centered
        ? bb.Center()  // Sphere centered in the bounding box
        : bb.ClosestPoint(custom_center); // Custom positioning

    // Generate the sphere primitive and associated octree
    auto& wrapper = manager.GetOctrees().at(selected_index);
    wrapper.octree = std::make_shared<Octree>(
        Octree::FromObject(bb, sphere(sphere_center, static_cast<double>(sphere_radius), mat()), depth_limit)
    );

    // Automatically rename the octree to reflect the sphere primitive
    manager.SetName(selected_index, "Sphere");

    // Generate the string representation of the octree
    octree_string = wrapper.octree->root.ToString();
    show_octree_string = true;
}


static void GenerateOctreeFromSquarePyramid(
    OctreeManager& manager,
    int selected_index,
    BoundingBox& bb,
    float pyramid_height,
    float pyramid_basis,
    bool centered,
    point3 custom_center,
    int depth_limit,
    std::string& octree_string,
    bool& show_octree_string
) {
    // Compute the pyramid's inferior point
    point3 pyramid_inferior_point = centered
        ? bb.Center() - point3(0, pyramid_height / 2.0, 0) // Centered in bounding box
        : bb.ClosestPoint(custom_center) - point3(0, pyramid_height / 2.0, 0); // Custom positioning

    SquarePyramid pyramid(pyramid_inferior_point, static_cast<double>(pyramid_height), static_cast<double>(pyramid_basis));

    auto& wrapper = manager.GetOctrees().at(selected_index);
    wrapper.octree = std::make_shared<Octree>(
        Octree::FromObject(bb, pyramid, depth_limit)
    );

    // Rename the octree using the OctreeManager
    manager.SetName(selected_index, "SquarePyramid");

    // Generate the string representation of the octree
    octree_string = wrapper.octree->root.ToString();
    show_octree_string = true;
}

static void GenerateOctreeFromCylinder(
    OctreeManager& manager,
    int selected_index,
    BoundingBox& bb,
    double height,
    double radius,
    bool centered,
    point3 custom_center,
    int depth_limit,
    std::string& octree_string,
    bool& show_octree_string
) {
    // Compute the cylinder's base center
    point3 base_center;
    if (centered) {
        // Base of the cylinder at the center of the bottom face of the bounding box
        base_center = bb.vmin + vec3(bb.width / 2.0, 0, bb.width / 2.0);
    }
    else {
        // Base on custom center
        base_center = bb.ClosestPoint(custom_center);
    }

    // Generate the cylinder primitive
    mat default_material = mat(color(1, 1, 1), 0); // Dummy material for the cylinder
    cylinder cyl(base_center, height, radius, default_material);

    // Generate the octree from the cylinder and the bounding box
    auto& wrapper = manager.GetOctrees().at(selected_index);
    wrapper.octree = std::make_shared<Octree>(
        Octree::FromObject(bb, cyl, depth_limit)
    );

    // Automatically rename the octree to reflect the cylinder primitive
    manager.SetName(selected_index, "Cylinder");

    // Generate the string representation of the octree
    octree_string = wrapper.octree->root.ToString();
    show_octree_string = true;
}

static void GenerateOctreeFromBox(
    OctreeManager& manager,
    int selected_index,
    BoundingBox& bb,
    float box_width,
    bool centered,
    point3 custom_center,
    int depth_limit,
    std::string& octree_string,
    bool& show_octree_string
) {
    // Compute the box's center
    point3 box_center = centered
        ? bb.Center()  // Box centered in the bounding box
        : custom_center; // Custom center

    // Generate the box primitive and associated octree
    auto& wrapper = manager.GetOctrees().at(selected_index);
    wrapper.octree = std::make_shared<Octree>(
        Octree::FromObject(bb, box(box_center, static_cast<double>(box_width), mat()), depth_limit)
    );

    // Automatically rename the octree to reflect the box primitive
    manager.SetName(selected_index, "Box");

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
    ImGui::SliderInt("Depth Limit", &manager.depth_limit, 1, 10);

    // Buttons for each Boolean operation
    if (ImGui::Button("Union (OR)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "union", manager.depth_limit);
    }
    ImGui::SameLine();
    if (ImGui::Button("Intersection (AND)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "intersection", manager.depth_limit);
    }
    ImGui::SameLine();
    if (ImGui::Button("Difference (NOT)")) {
        manager.PerformBooleanOperation(octree1_index, octree2_index, "difference", manager.depth_limit);
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
            manager.RebuildFromAnother(source_octree_index, depth_limit);
            ImGui::Text("Rebuild successful!");
        }
        catch (const std::exception& e) {
            ImGui::Text("Error: %s", e.what());
        }
    }
}
