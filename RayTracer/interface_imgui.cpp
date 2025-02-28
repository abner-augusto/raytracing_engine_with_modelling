#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"
#include <windows.h>
#include <commdlg.h>
#include <string>

#ifdef DIFFERENCE
#undef DIFFERENCE
#endif


extern bool renderWireframe;
extern bool renderWorldAxes;
extern bool renderWingedEdge;
extern bool renderWingedEdgeArrows;

// Function to open a file dialog and return the selected file path
std::string OpenFileDialog(const wchar_t* filter) {
    OPENFILENAMEW ofn; // Use wide-character version
    wchar_t file[MAX_PATH] = L""; // Wide-character string for file path

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        // Convert wide string to std::string
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, file, -1, NULL, 0, NULL, NULL);
        std::string converted_str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, file, -1, &converted_str[0], size_needed, NULL, NULL);
        return converted_str;
    }
    return "";
}

void draw_menu(RenderState& render_state, Camera& camera, SceneManager& world) {
    // Initialize static variables
    static bool isCameraSpace = camera.CameraSpaceStatus();
    static point3 previous_origin = camera.get_origin();
    static point3 previous_look_at = camera.get_look_at();
    static bool renderShadows = camera.shadowStatus();

    // Menu state tracking variables
    static bool cameraMenuOpen = false;
    static bool renderMenuOpen = false;
    static bool importMenuOpen = false;
    static bool projectionsMenuOpen = false;

    // Calculate total width needed for buttons
    float buttonSpacing = 5.0f;
    float cameraWidth = ImGui::CalcTextSize("Camera").x + 20.0f;  // Add padding
    float renderWidth = ImGui::CalcTextSize("Render").x + 20.0f;
    float importWidth = ImGui::CalcTextSize("Import").x + 20.0f;
    float totalWidth = cameraWidth + renderWidth + importWidth + (buttonSpacing * 2) + 10.0f;

    // Create a custom window to act as our menu bar
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(totalWidth, 0)); // 0 height for auto-sizing height
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    if (ImGui::Begin("##MenuBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize)) {

        // Create horizontal layout
        ImGui::BeginGroup();

        // Camera Menu Button
        if (ImGui::Button("Camera", ImVec2(cameraWidth, 0))) {
            cameraMenuOpen = !cameraMenuOpen;
            renderMenuOpen = false;
            importMenuOpen = false;
        }

        // Render Menu Button
        ImGui::SameLine(0, buttonSpacing);
        if (ImGui::Button("Render", ImVec2(renderWidth, 0))) {
            renderMenuOpen = !renderMenuOpen;
            cameraMenuOpen = false;
            importMenuOpen = false;
        }

        // Import Menu Button
        ImGui::SameLine(0, buttonSpacing);
        if (ImGui::Button("Import", ImVec2(importWidth, 0))) {
            importMenuOpen = !importMenuOpen;
            cameraMenuOpen = false;
            renderMenuOpen = false;
        }

        ImGui::EndGroup();

        ImGui::End();
    }

    ImGui::PopStyleVar(3); // Pop the style variables we pushed

    // Store the menu bar height for positioning dropdowns
    float menuBarHeight = ImGui::GetFrameHeightWithSpacing();

    // Camera Menu Content
    if (cameraMenuOpen) {
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
        if (ImGui::Begin("CameraMenu", &cameraMenuOpen,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {

            // Camera Origin controls
            ImGui::Text("Camera Origin (WASD Keys)");
            float origin_array[3] = {
                static_cast<float>(camera.get_origin().x()),
                static_cast<float>(camera.get_origin().y()),
                static_cast<float>(camera.get_origin().z())
            };

            ImGui::PushItemWidth(200);
            if (ImGui::SliderFloat3("Origin", origin_array, -10.0f, 10.0f)) {
                camera.set_origin(point3(origin_array[0], origin_array[1], origin_array[2]));
                //if (isCameraSpace) {
                //    world.transform(camera.world_to_camera_matrix);
                //}
            }

            // Look At Target controls
            ImGui::Text("Look At Target (Arrow Keys)");
            float target_array[3] = {
                static_cast<float>(camera.get_look_at().x()),
                static_cast<float>(camera.get_look_at().y()),
                static_cast<float>(camera.get_look_at().z())
            };
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
                //if (isCameraSpace) {
                //    world.transform(camera.world_to_camera_matrix);
                //}
            }

            // FOV and Orthographic Scale
            float camera_fov_degrees = static_cast<float>(camera.get_fov_degrees());
            if (ImGui::SliderFloat("FOV", &camera_fov_degrees, 10.0f, 120.0f)) {
                camera.set_fov(static_cast<double>(camera_fov_degrees));
            }

            float ortho_scale_float = static_cast<float>(camera.get_ortho_scale());
            if (ImGui::SliderFloat("Ortho Scale", &ortho_scale_float, 0.5f, 5.0f)) {
                camera.set_ortho_scale(static_cast<double>(ortho_scale_float));
            }
            ImGui::PopItemWidth();

            ImGui::Separator();

            // Integrated Projections Submenu using a Collapsing Header
            if (ImGui::CollapsingHeader("Projections")) {
                if (ImGui::Button("Perspective")) {
                    camera.set_origin(previous_origin);
                    camera.set_look_at(previous_look_at);
                    camera.use_perspective_projection();
                }
                if (ImGui::Button("Orthographic")) {
                    previous_origin = camera.get_origin();
                    previous_look_at = camera.get_look_at();
                    camera.use_orthographic_projection();
                }
                if (ImGui::Button("Isometric")) {
                    previous_origin = camera.get_origin();
                    previous_look_at = camera.get_look_at();
                    camera.set_origin(point3(1, 1.5, 1));
                    camera.set_look_at(point3(0, 0, 0));
                    camera.use_orthographic_projection();
                    camera.rotate_to_isometric_view();
                }
                if (ImGui::Button("Iso rotation")) {
                    camera.rotate_to_isometric_view();
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Reset to Default")) {
                if (isCameraSpace) {
                    camera.toggleCameraSpace();
                    isCameraSpace = false;
                    world.transform(camera.camera_to_world_matrix);
                }
                camera.set_origin(point3(0, 0, 2));
                camera.set_look_at(point3(0, 0, -3));
                camera.set_fov(60);
                camera.set_ortho_scale(1);

            }

            ImGui::Separator();
            if (ImGui::Checkbox("Camera Space", &isCameraSpace)) {
                camera.toggleCameraSpace();

                if (camera.CameraSpaceStatus()) {
                    std::cout << "Switching to Camera Space: Applying World to Camera Transform.\n";
                    world.transform(camera.world_to_camera_matrix);
                }
                else {
                    std::cout << "Switching to World Space: Applying Camera to World Transform.\n";
                    world.transform(camera.camera_to_world_matrix);
                }
            }

            ImGui::End();
        }
    }

    // Render Menu Content
    if (renderMenuOpen) {
        ImGui::SetNextWindowPos(ImVec2(cameraWidth + buttonSpacing, menuBarHeight));
        if (ImGui::Begin("RenderMenu", &renderMenuOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Quick render modes (1-4):");
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

            ImGui::Separator();

            if (ImGui::Checkbox("Toggle Shadows", &renderShadows)) {
                camera.toggleShadows();
            }
            bool wireframe = renderWireframe;
            if (ImGui::Checkbox("Toggle BB Wireframe", &wireframe)) {
                renderWireframe = wireframe;
            }
            bool worldAxes = renderWorldAxes;
            if (ImGui::Checkbox("Toggle World Axes", &worldAxes)) {
                renderWorldAxes = worldAxes;
            }
            bool wingedEdge = renderWingedEdge;
            if (ImGui::Checkbox("Toggle Winged Edge Wireframe", &wingedEdge)) {
                renderWingedEdge = wingedEdge;
            }
            bool wingedEdgeArrows = renderWingedEdgeArrows;
            if (ImGui::Checkbox("Toggle Edge Arrows", &wingedEdgeArrows)) {
                renderWingedEdgeArrows = wingedEdgeArrows;
            }

            ImGui::End();
        }
    }

    // Import Menu Content
    if (importMenuOpen) {
        ImGui::SetNextWindowPos(ImVec2(cameraWidth + renderWidth + (buttonSpacing * 2), menuBarHeight));
        if (ImGui::Begin("ImportMenu", &importMenuOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
            static std::string obj_filepath = "";
            static std::string mtl_filepath = "";
            static bool use_material = false;
            static float material_color[3] = { 1.0f, 1.0f, 1.0f };
            static mat default_material;

            if (ImGui::Button("Select OBJ File")) {
                std::string selected_file = OpenFileDialog(L"Wavefront OBJ (*.obj)\0*.obj\0");
                if (!selected_file.empty()) {
                    obj_filepath = selected_file;
                }
            }
            ImGui::Text("Selected OBJ: %s", obj_filepath.c_str());

            ImGui::Checkbox("Use MTL File", &use_material);
            if (use_material) {
                if (ImGui::Button("Select MTL File")) {
                    std::string selected_file = OpenFileDialog(L"Material File (*.mtl)\0*.mtl\0");
                    if (!selected_file.empty()) {
                        mtl_filepath = selected_file;
                    }
                }
                ImGui::Text("Selected MTL: %s", mtl_filepath.c_str());
            }
            else {
                ImGui::ColorEdit3("Material Color", material_color);
            }

            if (ImGui::Button("Import")) {
                try {
                    if (!use_material) {
                        default_material = mat(color(material_color[0], material_color[1], material_color[2]));
                    }
                    add_mesh_to_scene(obj_filepath, world, use_material ? mtl_filepath : "", default_material);
                    std::cout << "Successfully imported OBJ file: " << obj_filepath << std::endl;
                    world.buildBVH();
                }
                catch (const std::exception& e) {
                    std::cerr << "Error importing OBJ: " << e.what() << std::endl;
                }
            }

            ImGui::End();
        }
    }
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

// Use an optional to track selection (no selection if std::nullopt).
std::optional<ObjectID> selectedObjectID = std::nullopt;

void ShowHittableManagerUI(SceneManager& world, Camera& camera) {
    // Set window position to upper right corner
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screenSize.x, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGui::Begin("SceneManager Objects");

    // List all objects in the scene
    auto objectList = world.list_object_names();
    if (objectList.empty()) {
        ImGui::Text("No objects in the scene.");
    }
    else {
        ImGui::Text("SceneManager contains %zu object(s):", objectList.size());
        static size_t lastSelectedID = -1;
        static double lastClickTime = 0.0;

        for (const auto& [id, name] : objectList) {
            ImGui::PushID((int)id);
            bool isSelected = (selectedObjectID.has_value() && selectedObjectID.value() == id);
            if (ImGui::Selectable(name.c_str(), isSelected)) {
                // Track selection
                if (selectedObjectID.has_value() && selectedObjectID.value() == id) {
                    double currentTime = ImGui::GetTime();
                    if (lastSelectedID == id && (currentTime - lastClickTime) < 1.0) { // Double-click detected
                        auto object = world.get(id);
                        if (object) {
                            camera.set_look_at(object->bounding_box().getCenter());
                        }
                    }
                    lastClickTime = currentTime;
                }

                selectedObjectID = id;
                lastSelectedID = id;
            }
            ImGui::PopID();
        }
    }

    if (selectedObjectID.has_value()) {
        // Remove selected object button
        if (ImGui::Button("Remove Selected Object")) {
            if (selectedObjectID.value() == 0) {
                ImGui::OpenPopup("Cannot Delete Plane");
            }
            else {
                world.remove(selectedObjectID.value());
                selectedObjectID.reset();
                highlighted_box.reset();
            }
        }

        ImGui::SameLine(); // Ensure buttons are placed side by side

        // Look at Object button
        if (ImGui::Button("Look at Object")) {
            auto object = world.get(selectedObjectID.value());
            if (object) {
                camera.set_look_at(object->bounding_box().getCenter());
            }
        }

        // Popup message when trying to delete the infinite plane
        if (ImGui::BeginPopupModal("Cannot Delete Plane", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Cannot delete the infinite plane (ID 0).");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Get the position and size of the main window to anchor the properties window
    ImVec2 mainWindowPos = ImGui::GetWindowPos();
    ImVec2 mainWindowSize = ImGui::GetWindowSize();

    // Check if the main window is collapsed
    bool isCollapsed = ImGui::IsWindowCollapsed();

    // End the main window
    ImGui::End();

    if (isCollapsed) {
        return;
    }

    constexpr float paddingY = 5.0f;

    // Display Info Window below SceneManager Objects window
    ImGui::SetNextWindowPos(ImVec2(mainWindowPos.x, mainWindowPos.y + mainWindowSize.y + paddingY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainWindowSize.x, 0), ImGuiCond_Always);

    ShowInfoWindow(world);
}


std::optional<size_t> selectedLightIndex = std::nullopt;

void ShowLightsUI(SceneManager& world) {
    ImGui::Begin("SceneManager Lights");

    // Light list
    auto& lights = world.get_lights();
    if (lights.empty()) {
        ImGui::Text("No lights in the scene.");
    }
    else {
        for (size_t i = 0; i < lights.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            bool isSelected = (selectedLightIndex.has_value() && selectedLightIndex.value() == i);
            if (ImGui::Selectable(lights[i]->get_type_name().c_str(), isSelected)) {
                selectedLightIndex = i;
            }
            ImGui::PopID();
        }
        if (selectedLightIndex.has_value() && ImGui::Button("Remove Light")) {
            world.remove_light(selectedLightIndex.value());
            selectedLightIndex.reset();
        }
    }

    if (ImGui::BeginTabBar("LightTabs")) {
        // Tab for editing existing lights
        if (ImGui::BeginTabItem("Edit Lights")) {
            if (selectedLightIndex.has_value()) {
                auto& light = lights[selectedLightIndex.value()];
                ImGui::Separator();
                ImGui::Text("Edit Light Properties");

                if (!dynamic_cast<DirectionalLight*>(light.get())) {
                    vec3 pos = light->get_position();
                    double pos_min = -10.0, pos_max = 10.0;
                    if (ImGui::SliderScalarN("Position", ImGuiDataType_Double, pos.e, 3, &pos_min, &pos_max, "%.3f")) {
                        light->set_position(pos);
                    }
                }

                double intensity = light->get_intensity();
                double inten_min = 0.0, inten_max = 10.0;
                if (ImGui::SliderScalar("Intensity", ImGuiDataType_Double, &intensity, &inten_min, &inten_max, "%.3f")) {
                    light->set_intensity(intensity);
                }

                vec3 col = light->get_color();
                float col_f[3] = { static_cast<float>(col.e[0]), static_cast<float>(col.e[1]), static_cast<float>(col.e[2]) };
                if (ImGui::ColorEdit3("Color", col_f)) {
                    col.e[0] = static_cast<double>(col_f[0]);
                    col.e[1] = static_cast<double>(col_f[1]);
                    col.e[2] = static_cast<double>(col_f[2]);
                    light->set_color(col);
                }

                if (auto dirLight = dynamic_cast<DirectionalLight*>(light.get())) {
                    vec3 dir = dirLight->get_direction();
                    double dir_min = -1.0, dir_max = 1.0;
                    if (ImGui::SliderScalarN("Direction", ImGuiDataType_Double, dir.e, 3, &dir_min, &dir_max, "%.3f")) {
                        dirLight->set_direction(dir);
                    }
                }

                if (auto spotLight = dynamic_cast<SpotLight*>(light.get())) {
                    vec3 dir = spotLight->get_direction();
                    double dir_min = -1.0, dir_max = 1.0;
                    if (ImGui::SliderScalarN("Direction", ImGuiDataType_Double, dir.e, 3, &dir_min, &dir_max, "%.3f")) {
                        spotLight->set_direction(dir);
                    }

                    double inner = spotLight->get_inner_cutoff();
                    double outer = spotLight->get_outer_cutoff();
                    double cutoff_min = 0.0, cutoff_max = 90.0;
                    if (ImGui::SliderScalar("Inner Cutoff", ImGuiDataType_Double, &inner, &cutoff_min, &cutoff_max, "%.1f")) {
                        spotLight->set_cutoff_angles(inner, outer);
                    }
                    if (ImGui::SliderScalar("Outer Cutoff", ImGuiDataType_Double, &outer, &inner, &cutoff_max, "%.1f")) {
                        spotLight->set_cutoff_angles(inner, outer);
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // Tab for adding new lights
        if (ImGui::BeginTabItem("Add Lights")) {
            static int light_type = 0;
            ImGui::RadioButton("Point Light", &light_type, 0); ImGui::SameLine();
            ImGui::RadioButton("Directional Light", &light_type, 1); ImGui::SameLine();
            ImGui::RadioButton("Spot Light", &light_type, 2);

            static vec3 pos(0.0, 0.0, 0.0);
            static vec3 dir(0.0, -1.0, 0.0);
            static double intensity = 1.0;
            static vec3 color(1.0, 1.0, 1.0);
            static double cutoff = 30.0;
            static double outer_cutoff = 45.0;

            double pos_min = -10.0, pos_max = 10.0;
            ImGui::SliderScalarN("Position", ImGuiDataType_Double, pos.e, 3, &pos_min, &pos_max, "%.3f");

            if (light_type == 1 || light_type == 2) {
                double dir_min = -1.0, dir_max = 1.0;
                ImGui::SliderScalarN("Direction", ImGuiDataType_Double, dir.e, 3, &dir_min, &dir_max, "%.3f");
            }

            double inten_min = 0.0, inten_max = 10.0;
            ImGui::SliderScalar("Intensity", ImGuiDataType_Double, &intensity, &inten_min, &inten_max, "%.3f");

            float col_f[3] = { static_cast<float>(color.e[0]), static_cast<float>(color.e[1]), static_cast<float>(color.e[2]) };
            if (ImGui::ColorEdit3("Color", col_f)) {
                color.e[0] = static_cast<double>(col_f[0]);
                color.e[1] = static_cast<double>(col_f[1]);
                color.e[2] = static_cast<double>(col_f[2]);
            }

            if (light_type == 2) {
                double cutoff_min = 0.0, cutoff_max = 90.0;
                ImGui::SliderScalar("Inner Cutoff", ImGuiDataType_Double, &cutoff, &cutoff_min, &cutoff_max, "%.1f");
                ImGui::SliderScalar("Outer Cutoff", ImGuiDataType_Double, &outer_cutoff, &cutoff, &cutoff_max, "%.1f");
            }

            if (ImGui::Button("Add Light")) {
                if (light_type == 0) {
                    world.add_point_light(pos, intensity, color);
                }
                else if (light_type == 1) {
                    world.add_directional_light(dir, intensity, color);
                }
                else if (light_type == 2) {
                    world.add_spot_light(pos, dir, intensity, color, cutoff, outer_cutoff);
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Define a struct to hold all GUI constants.
struct GUIConstants {
    // Info Tab
    static constexpr int defaultOctreeDepth = 3;
    static constexpr int minOctreeDepth = 1;
    static constexpr int maxOctreeDepth = 6;

    // Geometry Tab
    static constexpr float translationMin = -2.0f;
    static constexpr float translationMax = 2.0f;
    // (Default translation values are zero.)

    // Box Primitive Tab
    static const std::array<float, 3> defaultBoxCenter;
    static constexpr float defaultBoxSize = 0.9f;
    static const std::array<float, 3> defaultVmin;
    static const std::array<float, 3> defaultVmax;
    static const std::array<float, 3> defaultBoxColor;

    // Sphere Primitive Tab
    static const std::array<float, 3> defaultSphereCenter;
    static constexpr float defaultSphereRadius = 0.6f;
    static const std::array<float, 3> defaultSphereColor;

    // Cylinder Primitive Tab
    static const std::array<float, 3> defaultCylinderBaseCenter;
    static const std::array<float, 3> defaultCylinderTopCenter;
    static constexpr float defaultCylinderRadius = 0.5f;
    static constexpr bool defaultCylinderCapped = true;
    static const std::array<float, 3> defaultCylinderColor;

    // Cone Primitive Tab
    static const std::array<float, 3> defaultConeBaseCenter;
    static const std::array<float, 3> defaultConeTopVertex;
    static constexpr float defaultConeRadius = 0.5f;
    static const std::array<float, 3> defaultConeColor;

    // Square Pyramid Primitive Tab
    static const std::array<float, 3> defaultPyramidBaseCenter;
    static constexpr float defaultPyramidHeight = 1.0f;
    static constexpr float defaultPyramidBaseSize = 1.0f;
    static const std::array<float, 3> defaultPyramidColor;
};

// Initialize the static arrays
const std::array<float, 3> GUIConstants::defaultBoxCenter = { 0.0f, 0.0f, -1.0f };
const std::array<float, 3> GUIConstants::defaultVmin = { -0.5f, -0.5f, -1.5f };
const std::array<float, 3> GUIConstants::defaultVmax = { 0.5f, 0.5f, -0.5f };
const std::array<float, 3> GUIConstants::defaultBoxColor = { 0.8f, 0.1f, 0.1f }; // Deep Red

const std::array<float, 3> GUIConstants::defaultSphereCenter = { 0.0f, 0.0f, -1.0f };
const std::array<float, 3> GUIConstants::defaultSphereColor = { 0.2f, 0.6f, 0.9f }; // Sky Blue

const std::array<float, 3> GUIConstants::defaultCylinderBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultCylinderTopCenter = { 0.0f, 0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultCylinderColor = { 0.3f, 0.85f, 0.3f }; // Lime Green

const std::array<float, 3> GUIConstants::defaultConeBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultConeTopVertex = { 0.0f, 0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultConeColor = { 0.6f, 0.3f, 0.8f }; // Violet

const std::array<float, 3> GUIConstants::defaultPyramidBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultPyramidColor = { 0.95f, 0.75f, 0.2f }; // Golden Yellow

// ---------------------------------------------------------------------
// Helper functions for each Tab

// Info Tab: Displays object information and octree controls.
void ShowInfoTab(SceneManager& world) {
    if (selectedObjectID.has_value()) {
        auto obj = world.get(selectedObjectID.value());
        if (obj) {
            ImGui::Text("Object ID: %zu", selectedObjectID.value());
            ImGui::Text("Object Type: %s", obj->get_type_name().c_str());

            static int octreeDepth = GUIConstants::defaultOctreeDepth;
            ImGui::SliderInt("Octree Depth", &octreeDepth, GUIConstants::minOctreeDepth, GUIConstants::maxOctreeDepth);

            if (ImGui::Button("Generate Octree")) {
                try {
                    world.generateObjectOctree(selectedObjectID.value(), octreeDepth);
                    std::cout << "[INFO] Octree generated for Object ID: " << selectedObjectID.value()
                        << " with depth " << octreeDepth << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[ERROR] Failed to generate octree: " << e.what() << std::endl;
                }
            }

            try {
                color diffuseColor = obj->get_material().diffuse_color;
                float color[3] = { static_cast<float>(diffuseColor.e[0]),
                                   static_cast<float>(diffuseColor.e[1]),
                                   static_cast<float>(diffuseColor.e[2]) };

                if (ImGui::ColorEdit3("Object Color", color)) {
                    diffuseColor.e[0] = static_cast<double>(color[0]);
                    diffuseColor.e[1] = static_cast<double>(color[1]);
                    diffuseColor.e[2] = static_cast<double>(color[2]);
                    obj->set_material(mat(diffuseColor));
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[ERROR] Failed to set material: " << e.what() << std::endl;
            }

            if (world.hasOctree(selectedObjectID.value())) {
                ImGui::Text("Octree Generated!");

                static double octreeVolume = 0.0;
                static double octreeSurfaceArea = 0.0;

                if (ImGui::Button("Calculate Volume")) {
                    try {
                        octreeVolume = world.getOctree(selectedObjectID.value()).volume();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Failed to calculate volume: " << e.what() << std::endl;
                    }
                }
                ImGui::SameLine();
                ImGui::Text("Volume: %.3f", octreeVolume);

                if (ImGui::Button("Calculate Surface Area")) {
                    try {
                        octreeSurfaceArea = world.getOctree(selectedObjectID.value()).CalculateHullSurfaceArea();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Failed to calculate surface area: " << e.what() << std::endl;
                    }
                }
                ImGui::SameLine();
                ImGui::Text("Area: %.3f", octreeSurfaceArea);

                if (ImGui::Button("Print Octree")) {
                    try {
                        world.getOctree(selectedObjectID.value()).root.ToHierarchicalString(
                            std::cout,
                            world.getOctree(selectedObjectID.value()).bounding_box
                        );
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Failed to print octree: " << e.what() << std::endl;
                    }
                }
            }
            else {
                ImGui::Text("No Octree Generated");
            }

            if (dynamic_cast<CSGPrimitive*>(obj.get()) ||
                dynamic_cast<CSGNode<Union>*>(obj.get()) ||
                dynamic_cast<CSGNode<Intersection>*>(obj.get()) ||
                dynamic_cast<CSGNode<Difference>*>(obj.get())) {
                if (ImGui::Button("Print CSG Tree")) {
                    print_csg_tree(obj);
                }
            }
        }
        else {
            ImGui::Text("Selected object not found.");
        }
    }
    else {
        ImGui::Text("No object selected.");
    }
}

// Geometry Tab: Allows object transformation.
void ShowGeometryTab(SceneManager& world) {
    if (!selectedObjectID.has_value()) {
        ImGui::Text("Select an object to transform it.");
        return;
    }

    auto obj = world.get(selectedObjectID.value());
    if (!obj) {
        ImGui::Text("Selected object not found.");
        return;
    }

    // Get the bounding box center of the object
    point3 center = obj->bounding_box().getCenter();
    ImGui::Text("Object Center: (%.2f, %.2f, %.2f)", center.x(), center.y(), center.z());

    ImGui::Separator();

    // Create a tab bar for transformations (Translation, Rotation, and Scaling)
    if (ImGui::BeginTabBar("TransformTabs")) {

        // Translation Tab
        if (ImGui::BeginTabItem("Translate")) {
            static float translation[3] = { 0.0f, 0.0f, 0.0f };

            ImGui::Text("Translate Object");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            ImGui::SliderFloat3(" ", translation, -2.0f, 2.0f);
            ImGui::PopItemWidth();

            if (ImGui::Button("Apply Translation")) {
                Matrix4x4 transform = Matrix4x4::translation(vec3(translation[0], translation[1], translation[2]));
                world.transform_object(selectedObjectID.value(), transform);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                translation[0] = translation[1] = translation[2] = 0.0f; // Reset
                world.buildBVH();
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Position")) {
                point3 targetPosition = point3(0.0f, 0.0f, -1.0f);
                vec3 resetTranslation = targetPosition - center;
                Matrix4x4 resetTransform = Matrix4x4::translation(resetTranslation);
                world.transform_object(selectedObjectID.value(), resetTransform);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
            }

            ImGui::EndTabItem();
        }

        // Rotation Tab
        if (ImGui::BeginTabItem("Rotate")) {
            static float rotationDirection[3] = { 0.0f, 1.0f, 0.0f }; // Default direction (Y-axis)
            static float rotationAngle = 0.0f;
            static bool isRotating = false;
            static float rotationSpeed = 1.0f; // Speed of rotation (1 to 10)
            static bool useCustomRotationPoint = false; // Checkbox for custom rotation point
            static float customRotationPoint[3] = { 0.0f, 0.0f, 0.0f }; // Custom rotation point

            ImGui::Text("Rotate Around Center");

            // Checkbox to toggle between center and custom rotation point
            ImGui::Checkbox("Use Custom Rotation Point", &useCustomRotationPoint);

            if (useCustomRotationPoint) {
                // Slider to define the custom rotation point
                ImGui::Text("Custom Rotation Point:");
                ImGui::SliderFloat3("Rotation Point", customRotationPoint, -10.0f, 10.0f);
            }

            ImGui::SliderFloat3("Rotation Axis", rotationDirection, -1.0f, 1.0f);
            ImGui::SliderFloat("Angle (degrees)", &rotationAngle, -180.0f, 180.0f);

            if (ImGui::Button("Apply Rotation")) {
                // Normalize the rotation axis to avoid scaling issues
                vec3 rotationAxis(rotationDirection[0], rotationDirection[1], rotationDirection[2]);
                if (rotationAxis.length_squared() > 0.0) {
                    rotationAxis = unit_vector(rotationAxis);

                    // Choose the rotation point based on the checkbox
                    point3 rotationPoint = useCustomRotationPoint ?
                        point3(customRotationPoint[0], customRotationPoint[1], customRotationPoint[2]) :
                        center;

                    Matrix4x4 rotationMatrix = rotationMatrix.rotateAroundPoint(rotationPoint, rotationAxis, rotationAngle);
                    world.transform_object(selectedObjectID.value(), rotationMatrix);
                    highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                    world.buildBVH();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Rotation")) {
                rotationDirection[0] = 0.0f; rotationDirection[1] = 1.0f; rotationDirection[2] = 0.0f;
                rotationAngle = 0.0f;
            }

            ImGui::Checkbox("Enable Rotation Animation", &isRotating);  // Checkbox to toggle rotation animation

            if (isRotating) {
                ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 1.0f, 10.0f); // Speed slider

                // Calculate the rotation angle based on the speed and frame time
                static float deltaTime = 0.0f;
                static auto lastTime = std::chrono::high_resolution_clock::now();
                auto currentTime = std::chrono::high_resolution_clock::now();
                deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
                lastTime = currentTime;

                float frameRotationAngle = rotationSpeed * deltaTime * 50.0f; // Adjust the multiplier for smooth rotation

                // Apply continuous rotation
                vec3 rotationAxis(rotationDirection[0], rotationDirection[1], rotationDirection[2]);
                if (rotationAxis.length_squared() > 0.0) {
                    rotationAxis = unit_vector(rotationAxis);

                    // Choose the rotation point based on the checkbox
                    point3 rotationPoint = useCustomRotationPoint ?
                        point3(customRotationPoint[0], customRotationPoint[1], customRotationPoint[2]) :
                        center;

                    Matrix4x4 rotationMatrix = rotationMatrix.rotateAroundPoint(rotationPoint, rotationAxis, frameRotationAngle);
                    world.transform_object(selectedObjectID.value(), rotationMatrix);
                    highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                    world.buildBVH(false);
                }
            }

            ImGui::EndTabItem();
        }

        // Scaling Tab
        if (ImGui::BeginTabItem("Scale")) {
            static float scaleValues[3] = { 1.0f, 1.0f, 1.0f }; // Default scale values
            static bool uniformScale = false; // Checkbox for uniform scaling
            static float uniformScaleValue = 1.0f; // Single slider value for uniform scaling
            static Matrix4x4 accumulatedScaleMatrix; // Store accumulated scaling transformation
            static bool firstScale = true;

            ImGui::Text("Scale Object");

            // Checkbox to toggle uniform scaling
            ImGui::Checkbox("Uniform Scaling", &uniformScale);

            if (uniformScale) {
                // Single slider for uniform scaling
                ImGui::SliderFloat("Scale All Axes", &uniformScaleValue, 0.1f, 5.0f);
                scaleValues[0] = scaleValues[1] = scaleValues[2] = uniformScaleValue;
            }
            else {
                // Separate sliders for each axis
                ImGui::SliderFloat3("Scale Axes", scaleValues, 0.1f, 5.0f);
            }

            if (ImGui::Button("Apply Scaling")) {
                // Apply scaling around the object's center
                Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-center.x(), -center.y(), -center.z()));
                Matrix4x4 scaleMatrix = Matrix4x4::scaling(scaleValues[0], scaleValues[1], scaleValues[2]);
                Matrix4x4 translateBack = Matrix4x4::translation(vec3(center.x(), center.y(), center.z()));
                Matrix4x4 finalTransform = translateBack * scaleMatrix * translateToOrigin;

                // Accumulate the scaling transformation
                if (firstScale) {
                    accumulatedScaleMatrix = finalTransform;
                    firstScale = false;
                }
                else {
                    accumulatedScaleMatrix = finalTransform * accumulatedScaleMatrix;
                }

                world.transform_object(selectedObjectID.value(), finalTransform);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                world.buildBVH();

                // Reset scale values for next input
                scaleValues[0] = scaleValues[1] = scaleValues[2] = 1.0f;
                uniformScaleValue = 1.0f;
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Scaling")) {
                try {
                    // Apply the inverse of the accumulated scaling matrix
                    Matrix4x4 inverseScale = accumulatedScaleMatrix.inverse();
                    world.transform_object(selectedObjectID.value(), inverseScale);
                    highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                    world.buildBVH();
                }
                catch (const std::runtime_error& e) {
                    std::cerr << "Error resetting scale: " << e.what() << "\n"; // Error handling
                }

                // Reset accumulated scaling matrix
                accumulatedScaleMatrix.set_identity();
                firstScale = true; // Reset for next accumulation
                scaleValues[0] = scaleValues[1] = scaleValues[2] = 1.0f;
                uniformScaleValue = 1.0f;
            }

            ImGui::EndTabItem();
        }


        // Shearing Tab
        if (ImGui::BeginTabItem("Shear")) {
            static float shearValues[3] = { 0.0f, 0.0f, 0.0f };
            static bool useCustomShearPoint = false;
            static float customShearPoint[3] = { 0.0f, 0.0f, 0.0f };
            static bool isShearing = false;
            static float shearSpeed = 1.0f;
            static float shearAmplitude = 1.0f;
            static float time = 0.0f;
            static Matrix4x4 accumulatedShearMatrix; // Matrix to store accumulated shear
            static bool firstShear = true;

            ImGui::Text("Shear Object");
            ImGui::Checkbox("Use Custom Shearing Point", &useCustomShearPoint);

            if (useCustomShearPoint) {
                ImGui::Text("Custom Shearing Point:");
                ImGui::SliderFloat3("Shearing Point", customShearPoint, -10.0f, 10.0f);
            }

            ImGui::SliderFloat3("Shear Factors", shearValues, -1.0f, 1.0f);

            if (ImGui::Button("Apply Shear")) {
                point3 shearingPoint = useCustomShearPoint ?
                    point3(customShearPoint[0], customShearPoint[1], customShearPoint[2]) :
                    center;
                Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-shearingPoint.x(), -shearingPoint.y(), -shearingPoint.z()));
                Matrix4x4 shearMatrix = Matrix4x4::shearing(shearValues[0], shearValues[1], shearValues[2]);
                Matrix4x4 translateBack = Matrix4x4::translation(vec3(shearingPoint.x(), shearingPoint.y(), shearingPoint.z()));
                Matrix4x4 finalTransform = translateBack * shearMatrix * translateToOrigin;

                // Accumulate the shearing transformation
                if (firstShear) {
                    accumulatedShearMatrix = finalTransform;
                    firstShear = false;
                }
                else {
                    accumulatedShearMatrix = finalTransform * accumulatedShearMatrix;
                }

                world.transform_object(selectedObjectID.value(), finalTransform);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                shearValues[0] = shearValues[1] = shearValues[2] = 0.0f;
                world.buildBVH();
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Shear")) {
                // Apply the inverse of the accumulated shear matrix
                try {
                    Matrix4x4 inverseShear = accumulatedShearMatrix.inverse();
                    world.transform_object(selectedObjectID.value(), inverseShear);
                    highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                    world.buildBVH();
                }
                catch (const std::runtime_error& e) {
                    std::cerr << "Error resetting shear: " << e.what() << "\n"; //error handling
                }
                // Reset the accumulated shear matrix and shear values
                accumulatedShearMatrix.set_identity();
                shearValues[0] = shearValues[1] = shearValues[2] = 0.0f;
                firstShear = true; // Reset for next accumulation
            }

            ImGui::Separator();
            ImGui::Checkbox("Enable Shear Animation", &isShearing);

            if (isShearing) {
                ImGui::SliderFloat("Shear Speed", &shearSpeed, 0.1f, 5.0f);
                ImGui::SliderFloat("Shear Amplitude", &shearAmplitude, 0.1f, 2.0f);

                static float deltaTime = 0.0f;
                static auto lastTime = std::chrono::high_resolution_clock::now();
                auto currentTime = std::chrono::high_resolution_clock::now();
                deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
                lastTime = currentTime;

                time += deltaTime * shearSpeed;
                float shearFactor = shearAmplitude * std::sin(time * 2 * pi);

                point3 shearingPoint = useCustomShearPoint ?
                    point3(customShearPoint[0], customShearPoint[1], customShearPoint[2]) : center;

                Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-shearingPoint.x(), -shearingPoint.y(), -shearingPoint.z()));
                Matrix4x4 shearMatrix = Matrix4x4::shearing(shearFactor * shearValues[0], shearFactor * shearValues[1], shearFactor * shearValues[2]);
                Matrix4x4 translateBack = Matrix4x4::translation(vec3(shearingPoint.x(), shearingPoint.y(), shearingPoint.z()));
                Matrix4x4 finalTransform = translateBack * shearMatrix * translateToOrigin;

                // Accumulate the shearing transformation for animation
                if (firstShear) {
                    accumulatedShearMatrix = finalTransform;
                    firstShear = false;
                }
                else {
                    accumulatedShearMatrix = finalTransform * accumulatedShearMatrix;
                }

                world.transform_object(selectedObjectID.value(), finalTransform);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                world.buildBVH(false);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// Primitives Tab: Displays sub-tabs for creating different primitives.
void ShowPrimitivesTab(SceneManager& world) {
    if (ImGui::BeginTabBar("PrimitiveTabs")) {
        // ----- Box Tab -----
        if (ImGui::BeginTabItem("Box")) {
            static bool useMinMax = false;
            static float boxCenter[3] = { GUIConstants::defaultBoxCenter[0],
                                          GUIConstants::defaultBoxCenter[1],
                                          GUIConstants::defaultBoxCenter[2] };
            static float boxSize = GUIConstants::defaultBoxSize;
            static float vmin[3] = { GUIConstants::defaultVmin[0],
                                     GUIConstants::defaultVmin[1],
                                     GUIConstants::defaultVmin[2] };
            static float vmax[3] = { GUIConstants::defaultVmax[0],
                                     GUIConstants::defaultVmax[1],
                                     GUIConstants::defaultVmax[2] };
            static float boxColor[3] = { GUIConstants::defaultBoxColor[0],
                                         GUIConstants::defaultBoxColor[1],
                                         GUIConstants::defaultBoxColor[2] };

            ImGui::Checkbox("Use Vmin/Vmax", &useMinMax);

            if (useMinMax) {
                ImGui::SliderFloat3("Vmin", vmin, GUIConstants::translationMin, GUIConstants::translationMax);
                ImGui::SliderFloat3("Vmax", vmax, GUIConstants::translationMin, GUIConstants::translationMax);
            }
            else {
                ImGui::SliderFloat3("Center", boxCenter, GUIConstants::translationMin, GUIConstants::translationMax);
                ImGui::SliderFloat("Size", &boxSize, 0.1f, 5.0f);
            }
            ImGui::ColorEdit3("Color", boxColor);

            if (ImGui::Button("Create Box")) {
                std::shared_ptr<CSGPrimitive> boxPrim;
                if (useMinMax) {
                    boxPrim = std::make_shared<CSGPrimitive>(
                        std::make_shared<box_csg>(
                            point3(vmin[0], vmin[1], vmin[2]),
                            point3(vmax[0], vmax[1], vmax[2]),
                            mat(color(boxColor[0], boxColor[1], boxColor[2]))
                        )
                    );
                }
                else {
                    boxPrim = std::make_shared<CSGPrimitive>(
                        std::make_shared<box_csg>(
                            point3(boxCenter[0], boxCenter[1], boxCenter[2]),
                            boxSize,
                            mat(color(boxColor[0], boxColor[1], boxColor[2]))
                        )
                    );
                }
                ObjectID newID = world.add(boxPrim);
                selectedObjectID = newID;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                boxCenter[0] = GUIConstants::defaultBoxCenter[0];
                boxCenter[1] = GUIConstants::defaultBoxCenter[1];
                boxCenter[2] = GUIConstants::defaultBoxCenter[2];
                boxSize = GUIConstants::defaultBoxSize;
                vmin[0] = GUIConstants::defaultVmin[0];
                vmin[1] = GUIConstants::defaultVmin[1];
                vmin[2] = GUIConstants::defaultVmin[2];
                vmax[0] = GUIConstants::defaultVmax[0];
                vmax[1] = GUIConstants::defaultVmax[1];
                vmax[2] = GUIConstants::defaultVmax[2];
                boxColor[0] = GUIConstants::defaultBoxColor[0];
                boxColor[1] = GUIConstants::defaultBoxColor[1];
                boxColor[2] = GUIConstants::defaultBoxColor[2];
            }
            ImGui::EndTabItem();
        }

        // ----- Sphere Tab -----
        if (ImGui::BeginTabItem("Sphere")) {
            static float sphereCenter[3] = { GUIConstants::defaultSphereCenter[0],
                                             GUIConstants::defaultSphereCenter[1],
                                             GUIConstants::defaultSphereCenter[2] };
            static float initialSphereCenter[3] = { GUIConstants::defaultSphereCenter[0],
                                                    GUIConstants::defaultSphereCenter[1],
                                                    GUIConstants::defaultSphereCenter[2] };
            static float sphereRadius = GUIConstants::defaultSphereRadius;
            static float initialSphereRadius = GUIConstants::defaultSphereRadius;
            static float sphereColor[3] = { GUIConstants::defaultSphereColor[0],
                                            GUIConstants::defaultSphereColor[1],
                                            GUIConstants::defaultSphereColor[2] };
            static float initialSphereColor[3] = { GUIConstants::defaultSphereColor[0],
                                                   GUIConstants::defaultSphereColor[1],
                                                   GUIConstants::defaultSphereColor[2] };

            ImGui::SliderFloat3("Center", sphereCenter, GUIConstants::translationMin, GUIConstants::translationMax);
            ImGui::SliderFloat("Radius", &sphereRadius, 0.1f, 3.0f);
            ImGui::ColorEdit3("Color", sphereColor);

            if (ImGui::Button("Create Sphere")) {
                auto spherePrim = std::make_shared<CSGPrimitive>(
                    std::make_shared<sphere>(
                        point3(sphereCenter[0], sphereCenter[1], sphereCenter[2]),
                        sphereRadius,
                        mat(color(sphereColor[0], sphereColor[1], sphereColor[2]))
                    )
                );
                ObjectID newID = world.add(spherePrim);
                selectedObjectID = newID;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                memcpy(sphereCenter, initialSphereCenter, sizeof(sphereCenter));
                sphereRadius = initialSphereRadius;
                memcpy(sphereColor, initialSphereColor, sizeof(sphereColor));
            }
            ImGui::EndTabItem();
        }

        // ----- Cylinder Tab -----
        if (ImGui::BeginTabItem("Cylinder")) {
            static float baseCenter[3] = { GUIConstants::defaultCylinderBaseCenter[0],
                                           GUIConstants::defaultCylinderBaseCenter[1],
                                           GUIConstants::defaultCylinderBaseCenter[2] };
            static float topCenter[3] = { GUIConstants::defaultCylinderTopCenter[0],
                                          GUIConstants::defaultCylinderTopCenter[1],
                                          GUIConstants::defaultCylinderTopCenter[2] };
            static float radius = GUIConstants::defaultCylinderRadius;
            static bool capped = GUIConstants::defaultCylinderCapped;
            static float cylinderColor[3] = { GUIConstants::defaultCylinderColor[0],
                                              GUIConstants::defaultCylinderColor[1],
                                              GUIConstants::defaultCylinderColor[2] };

            ImGui::SliderFloat3("Base Center", baseCenter, -5.0f, 5.0f);
            ImGui::SliderFloat3("Top Center", topCenter, -5.0f, 5.0f);
            ImGui::SliderFloat("Radius", &radius, 0.1f, 2.0f);
            ImGui::ColorEdit3("Color", cylinderColor);
            ImGui::Checkbox("Capped", &capped);

            if (ImGui::Button("Create Cylinder")) {
                auto cylinderPrim = std::make_shared<CSGPrimitive>(
                    std::make_shared<cylinder>(
                        point3(baseCenter[0], baseCenter[1], baseCenter[2]),
                        point3(topCenter[0], topCenter[1], topCenter[2]),
                        radius,
                        mat(color(cylinderColor[0], cylinderColor[1], cylinderColor[2])),
                        capped
                    )
                );
                ObjectID newID = world.add(cylinderPrim);
                selectedObjectID = newID;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                baseCenter[0] = GUIConstants::defaultCylinderBaseCenter[0];
                baseCenter[1] = GUIConstants::defaultCylinderBaseCenter[1];
                baseCenter[2] = GUIConstants::defaultCylinderBaseCenter[2];
                topCenter[0] = GUIConstants::defaultCylinderTopCenter[0];
                topCenter[1] = GUIConstants::defaultCylinderTopCenter[1];
                topCenter[2] = GUIConstants::defaultCylinderTopCenter[2];
                radius = GUIConstants::defaultCylinderRadius;
                capped = GUIConstants::defaultCylinderCapped;
                cylinderColor[0] = GUIConstants::defaultCylinderColor[0];
                cylinderColor[1] = GUIConstants::defaultCylinderColor[1];
                cylinderColor[2] = GUIConstants::defaultCylinderColor[2];
            }
            ImGui::EndTabItem();
        }

        // ----- Cone Tab -----
        if (ImGui::BeginTabItem("Cone")) {
            static float baseCenter[3] = { GUIConstants::defaultConeBaseCenter[0],
                                           GUIConstants::defaultConeBaseCenter[1],
                                           GUIConstants::defaultConeBaseCenter[2] };
            static float topVertex[3] = { GUIConstants::defaultConeTopVertex[0],
                                          GUIConstants::defaultConeTopVertex[1],
                                          GUIConstants::defaultConeTopVertex[2] };
            static float radius = GUIConstants::defaultConeRadius;
            static bool capped = true; // For cone, if needed
            static float coneColor[3] = { GUIConstants::defaultConeColor[0],
                                          GUIConstants::defaultConeColor[1],
                                          GUIConstants::defaultConeColor[2] };

            ImGui::SliderFloat3("Base Center", baseCenter, GUIConstants::translationMin, GUIConstants::translationMax);
            ImGui::SliderFloat3("Top Vertex", topVertex, GUIConstants::translationMin, GUIConstants::translationMax);
            ImGui::SliderFloat("Radius", &radius, 0.1f, 2.0f);
            ImGui::ColorEdit3("Color", coneColor);
            ImGui::Checkbox("Capped", &capped);

            if (ImGui::Button("Create Cone")) {
                auto conePrim = std::make_shared<CSGPrimitive>(
                    std::make_shared<cone>(
                        point3(baseCenter[0], baseCenter[1], baseCenter[2]),
                        point3(topVertex[0], topVertex[1], topVertex[2]),
                        radius,
                        mat(color(coneColor[0], coneColor[1], coneColor[2]))
                    )
                );
                ObjectID newID = world.add(conePrim);
                selectedObjectID = newID;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                baseCenter[0] = GUIConstants::defaultConeBaseCenter[0];
                baseCenter[1] = GUIConstants::defaultConeBaseCenter[1];
                baseCenter[2] = GUIConstants::defaultConeBaseCenter[2];
                topVertex[0] = GUIConstants::defaultConeTopVertex[0];
                topVertex[1] = GUIConstants::defaultConeTopVertex[1];
                topVertex[2] = GUIConstants::defaultConeTopVertex[2];
                radius = GUIConstants::defaultConeRadius;
                capped = true;
                coneColor[0] = GUIConstants::defaultConeColor[0];
                coneColor[1] = GUIConstants::defaultConeColor[1];
                coneColor[2] = GUIConstants::defaultConeColor[2];
            }
            ImGui::EndTabItem();
        }

        // ----- Square Pyramid Tab -----
        if (ImGui::BeginTabItem("Square Pyramid")) {
            static float baseCenter[3] = { GUIConstants::defaultPyramidBaseCenter[0],
                                           GUIConstants::defaultPyramidBaseCenter[1],
                                           GUIConstants::defaultPyramidBaseCenter[2] };
            static float height = GUIConstants::defaultPyramidHeight;
            static float baseSize = GUIConstants::defaultPyramidBaseSize;
            static float pyramidColor[3] = { GUIConstants::defaultPyramidColor[0],
                                             GUIConstants::defaultPyramidColor[1],
                                             GUIConstants::defaultPyramidColor[2] };

            ImGui::SliderFloat3("Base Center", baseCenter, GUIConstants::translationMin, GUIConstants::translationMax);
            ImGui::SliderFloat("Height", &height, 0.1f, 2.0f);
            ImGui::SliderFloat("Base Size", &baseSize, 0.1f, 2.0f);
            ImGui::ColorEdit3("Color", pyramidColor);

            if (ImGui::Button("Create Square Pyramid")) {
                auto pyramidPrim = std::make_shared<CSGPrimitive>(
                    std::make_shared<SquarePyramid>(
                        point3(baseCenter[0], baseCenter[1], baseCenter[2]),
                        height,
                        baseSize,
                        mat(color(pyramidColor[0], pyramidColor[1], pyramidColor[2]))
                    )
                );
                ObjectID newID = world.add(pyramidPrim);
                selectedObjectID = newID;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                baseCenter[0] = GUIConstants::defaultPyramidBaseCenter[0];
                baseCenter[1] = GUIConstants::defaultPyramidBaseCenter[1];
                baseCenter[2] = GUIConstants::defaultPyramidBaseCenter[2];
                height = GUIConstants::defaultPyramidHeight;
                baseSize = GUIConstants::defaultPyramidBaseSize;
                pyramidColor[0] = GUIConstants::defaultPyramidColor[0];
                pyramidColor[1] = GUIConstants::defaultPyramidColor[1];
                pyramidColor[2] = GUIConstants::defaultPyramidColor[2];
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// Boolean Tab: Allows for selecting two objects and applying a CSG Boolean operation.
void ShowBooleanTab(SceneManager& world) {
    static std::optional<ObjectID> leftObjectID = std::nullopt;
    static std::optional<ObjectID> rightObjectID = std::nullopt;
    static CSGType selectedOperation = CSGType::UNION;

    auto objectList = world.list_object_names();
    std::string leftDisplay = "None";
    if (leftObjectID.has_value()) {
        for (const auto& [id, name] : objectList) {
            if (id == leftObjectID.value()) {
                leftDisplay = name;
                break;
            }
        }
    }
    std::string rightDisplay = "None";
    if (rightObjectID.has_value()) {
        for (const auto& [id, name] : objectList) {
            if (id == rightObjectID.value()) {
                rightDisplay = name;
                break;
            }
        }
    }

    ImGui::Text("Select First Object:");
    if (ImGui::BeginCombo("##FirstObject", leftDisplay.c_str())) {
        for (const auto& [id, name] : objectList) {
            auto obj = world.get(id);
            if (!obj) continue;
            if (!dynamic_cast<CSGNode<Union>*>(obj.get()) &&
                !dynamic_cast<CSGNode<Intersection>*>(obj.get()) &&
                !dynamic_cast<CSGNode<Difference>*>(obj.get()) &&
                !dynamic_cast<CSGPrimitive*>(obj.get()))
                continue;
            bool isSelected = (leftObjectID.has_value() && leftObjectID.value() == id);
            if (ImGui::Selectable(name.c_str(), isSelected))
                leftObjectID = id;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Select Second Object:");
    if (ImGui::BeginCombo("##SecondObject", rightDisplay.c_str())) {
        for (const auto& [id, name] : objectList) {
            auto obj = world.get(id);
            if (!obj) continue;
            if (!dynamic_cast<CSGNode<Union>*>(obj.get()) &&
                !dynamic_cast<CSGNode<Intersection>*>(obj.get()) &&
                !dynamic_cast<CSGNode<Difference>*>(obj.get()) &&
                !dynamic_cast<CSGPrimitive*>(obj.get()))
                continue;
            bool isSelected = (rightObjectID.has_value() && rightObjectID.value() == id);
            if (ImGui::Selectable(name.c_str(), isSelected))
                rightObjectID = id;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Select Operation:");
    if (ImGui::BeginCombo("##Operation", csg_type_to_string(selectedOperation).c_str())) {
        if (ImGui::Selectable("Union", selectedOperation == CSGType::UNION))
            selectedOperation = CSGType::UNION;
        if (ImGui::Selectable("Intersection", selectedOperation == CSGType::INTERSECTION))
            selectedOperation = CSGType::INTERSECTION;
        if (ImGui::Selectable("Difference", selectedOperation == CSGType::DIFFERENCE))
            selectedOperation = CSGType::DIFFERENCE;
        ImGui::EndCombo();
    }

    if (ImGui::Button("Apply Boolean Operation")) {
        if (leftObjectID.has_value() && rightObjectID.has_value()) {
            auto leftObject = world.get(leftObjectID.value());
            auto rightObject = world.get(rightObjectID.value());
            if (leftObject && rightObject) {
                std::shared_ptr<hittable> csgNode;
                switch (selectedOperation) {
                case CSGType::UNION:
                    csgNode = std::make_shared<CSGNode<Union>>(leftObject, rightObject);
                    break;
                case CSGType::INTERSECTION:
                    csgNode = std::make_shared<CSGNode<Intersection>>(leftObject, rightObject);
                    break;
                case CSGType::DIFFERENCE:
                    csgNode = std::make_shared<CSGNode<Difference>>(leftObject, rightObject);
                    break;
                default:
                    break;
                }
                if (csgNode) {
                    // Reset global selection if needed.
                    if (selectedObjectID.has_value() &&
                        (selectedObjectID.value() == leftObjectID.value() ||
                            selectedObjectID.value() == rightObjectID.value())) {
                        // Reset selection.
                    }
                    ObjectID newID = world.add(csgNode);
                    selectedObjectID = newID;
                    world.remove(leftObjectID.value());
                    world.remove(rightObjectID.value());
                    leftObjectID.reset();
                    rightObjectID.reset();
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
// Main window function that calls the helper functions in a TabBar.
void ShowInfoWindow(SceneManager& world) {
    ImGui::Begin("Object Properties");
    if (ImGui::BeginTabBar("NodeWindowTabBar")) {
        if (ImGui::BeginTabItem("Info")) {
            ShowInfoTab(world);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Geometry")) {
            ShowGeometryTab(world);
            ImGui::EndTabItem();
        }
        //if (ImGui::BeginTabItem("Add Objects")) {
        //    ShowPrimitivesTab(world);
        //    ImGui::EndTabItem();
        //}
        //if (ImGui::BeginTabItem("Boolean")) {
        //    ShowBooleanTab(world);
        //    ImGui::EndTabItem();
        //}
        ImGui::EndTabBar();
    }

    ImVec2 infoWindowPos = ImGui::GetWindowPos();
    ImVec2 infoWindowSize = ImGui::GetWindowSize();
    bool isCollapsed = ImGui::IsWindowCollapsed();
    ImGui::End();  // End Info window

    constexpr float paddingY = 5.0f;

    if (!isCollapsed) {
        // Position Lights UI below the Info window when expanded
        ImGui::SetNextWindowPos(ImVec2(infoWindowPos.x, infoWindowPos.y + infoWindowSize.y + paddingY), ImGuiCond_Always);
    }
    else {
        // When Info window is collapsed, position Lights UI right below the title bar
        float titleBarHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2;
        ImGui::SetNextWindowPos(ImVec2(infoWindowPos.x, infoWindowPos.y + titleBarHeight + paddingY), ImGuiCond_Always);
    }

    // Set width aligned with the left side of the Info window
    ImGui::SetNextWindowSize(ImVec2(infoWindowSize.x, 0), ImGuiCond_Always);

    ShowLightsUI(world);
}