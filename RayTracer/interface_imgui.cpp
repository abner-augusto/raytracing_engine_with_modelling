#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"


extern bool renderWireframe;
extern bool renderWorldAxes;

void draw_menu(RenderState& render_state, Camera& camera, HittableManager& world)

{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {

        static bool isCameraSpace = false;
        static point3 previous_origin = camera.get_origin();  // Store previous origin
        static point3 previous_look_at = camera.get_look_at();  // Store previous look-at

        // Camera Header
        if (ImGui::CollapsingHeader("Camera")) {
            //ImGui::Checkbox("Camera Space Transform", &isCameraSpace);

            // Camera Origin with keyboard control info
            ImGui::Text("Camera Origin (WASD Keys)");
            float origin_array[3] = {
                static_cast<float>(camera.get_origin().x()),
                static_cast<float>(camera.get_origin().y()),
                static_cast<float>(camera.get_origin().z())
            };

            // Fixed width for sliders
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            if (ImGui::SliderFloat3("Origin", origin_array, -10.0f, 10.0f)) {
                camera.set_origin(point3(origin_array[0], origin_array[1], origin_array[2]));
                if (isCameraSpace) {
                    world.transform(camera.world_to_camera_matrix);
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
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
                if (isCameraSpace) {
                    world.transform(camera.world_to_camera_matrix);
                }
            }
            ImGui::PopItemWidth();

            float camera_fov_degrees = static_cast<float>(camera.get_fov_degrees());
            ImGui::Text("Camera FOV");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            if (ImGui::SliderFloat("FOV", &camera_fov_degrees, 10.0f, 120.0f)) {
                camera.set_fov(static_cast<double>(camera_fov_degrees));
            }
            ImGui::PopItemWidth();

            if (ImGui::Button("Reset to Default")) {
                camera.set_origin(point3(0, 0, 0));
                camera.set_look_at(point3(0, 0, -3));
                camera.set_fov(60);
                if (isCameraSpace) {
                    world.transform(camera.world_to_camera_matrix);
                }
            }
            ImGui::Separator();
            ImGui::Text("Camera Projections:");

            if (ImGui::Button("Perspective")) {
                // Restore previous camera position before switching to perspective
                camera.set_origin(previous_origin);
                camera.set_look_at(previous_look_at);
                camera.use_perspective_projection();
            }
            ImGui::SameLine();
            if (ImGui::Button("Orthographic")) {
                previous_origin = camera.get_origin();
                previous_look_at = camera.get_look_at();
                camera.use_orthographic_projection();
            }

            if (ImGui::Button("Isometric")) {
                // Store current camera position before switching to orthographic
                previous_origin = camera.get_origin();
                previous_look_at = camera.get_look_at();
                camera.set_origin(point3(1, 1.5, 1));
                camera.set_look_at(point3(0, 0, 0));
                camera.use_orthographic_projection();
                camera.rotate_to_isometric_view();
            }
            ImGui::SameLine();
            if (ImGui::Button("Iso rotation")) {
                camera.rotate_to_isometric_view();
            }

            ImGui::Separator();

            ImGui::Text("Hint: Camera is now controllable");
            ImGui::Text("by the gamepad as well!");
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
                camera.toggleShadows();
            }

            if (ImGui::Checkbox("Toggle Wireframe", &renderWireframe)) {

            }

            if (ImGui::Checkbox("Toggle World Axes", &renderWorldAxes)) {

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

// Use an optional to track selection (no selection if std::nullopt).
std::optional<ObjectID> selectedObjectID = std::nullopt;

void ShowHittableManagerUI(HittableManager& world) {
    // Set window position to upper right corner
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screenSize.x - 10, 10), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));

    ImGui::Begin("Scene Objects");

    // List all objects in the scene
    auto objectList = world.list_object_names();
    if (objectList.empty()) {
        ImGui::Text("No objects in the scene.");
    }
    else {
        ImGui::Text("Scene contains %d object(s):", (int)objectList.size());
        for (const auto& [id, name] : objectList) {
            ImGui::PushID((int)id);
            bool isSelected = (selectedObjectID.has_value() && selectedObjectID.value() == id);
            if (ImGui::Selectable(name.c_str(), isSelected)) {
                selectedObjectID = id;
            }
            ImGui::PopID();
        }
    }

    // Remove selected object button
    if (selectedObjectID.has_value()) {
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

    // If the main window is collapsed, do not show the info window
    if (isCollapsed) {
        return;
    }

    // Define padding between the two windows
    constexpr float paddingY = 10.0f; // Adjust as needed for better spacing

    // Set the position of the properties window to be below the main window with padding
    ImGui::SetNextWindowPos(
        ImVec2(mainWindowPos.x, mainWindowPos.y + mainWindowSize.y + paddingY),
        ImGuiCond_Always
    );

    // Ensure the width of ShowInfoWindow matches the Scene Objects window
    ImGui::SetNextWindowSize(
        ImVec2(mainWindowSize.x, 0), // Width matches main window, height auto-adjusts
        ImGuiCond_Always
    );

    // Show the properties window only if the main window is not collapsed
    ShowInfoWindow(world);
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

// Initialize the static arrays (typically in a .cpp file)
const std::array<float, 3> GUIConstants::defaultBoxCenter = { 0.0f, 0.0f, -1.0f };
const std::array<float, 3> GUIConstants::defaultVmin = { -0.5f, -0.5f, -1.5f };
const std::array<float, 3> GUIConstants::defaultVmax = { 0.5f, 0.5f, -0.5f };
const std::array<float, 3> GUIConstants::defaultBoxColor = { 1.0f, 0.0f, 0.0f };

const std::array<float, 3> GUIConstants::defaultSphereCenter = { 0.0f, 0.0f, -1.0f };
const std::array<float, 3> GUIConstants::defaultSphereColor = { 0.0f, 0.0f, 1.0f };

const std::array<float, 3> GUIConstants::defaultCylinderBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultCylinderTopCenter = { 0.0f, 0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultCylinderColor = { 0.0f, 0.5f, 1.0f };

const std::array<float, 3> GUIConstants::defaultConeBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultConeTopVertex = { 0.0f, 0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultConeColor = { 1.0f, 0.5f, 0.0f };

const std::array<float, 3> GUIConstants::defaultPyramidBaseCenter = { 0.0f, -0.5f, -1.0f };
const std::array<float, 3> GUIConstants::defaultPyramidColor = { 0.8f, 0.3f, 0.0f };

// ---------------------------------------------------------------------
// Helper functions for each Tab

// Info Tab: Displays object information and octree controls.
void ShowInfoTab(HittableManager& world) {
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
                        // Use the . operator to access members of the Octree reference
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

            // Check if the object is a CSGNode or CSGPrimitive for CSG tree printing.
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
void ShowGeometryTab(HittableManager& world) {
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
                }
            }

            ImGui::EndTabItem();
        }

        // Scaling Tab
        if (ImGui::BeginTabItem("Scale")) {
            static float scaleValues[3] = { 1.0f, 1.0f, 1.0f }; // Default scale values
            static bool uniformScale = false; // Checkbox for uniform scaling
            static float uniformScaleValue = 1.0f; // Single slider value for uniform scaling

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
                Matrix4x4 scaleMatrix = scaleMatrix.scaleAroundPoint(center, scaleValues[0], scaleValues[1], scaleValues[2]);
                world.transform_object(selectedObjectID.value(), scaleMatrix);
                highlighted_box = world.get(selectedObjectID.value())->bounding_box();
                scaleValues[0] = scaleValues[1] = scaleValues[2] = 1.0f;
                uniformScaleValue = 1.0f;
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Scaling")) {
                scaleValues[0] = scaleValues[1] = scaleValues[2] = 1.0f; // Reset to default scale
                uniformScaleValue = 1.0f;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}


// Primitives Tab: Displays sub-tabs for creating different primitives.
void ShowPrimitivesTab(HittableManager& world) {
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
void ShowBooleanTab(HittableManager& world) {
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
void ShowInfoWindow(HittableManager& world) {
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
    ImGui::End();
}
