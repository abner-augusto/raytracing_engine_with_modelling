#define _CRT_SECURE_NO_WARNINGS
#include "interface_imgui.h"


extern bool renderWireframe;

void draw_menu(RenderState& render_state,
    Camera& camera, HittableManager world, std::vector<Light>& lights)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {

        static bool isCameraSpace = false;

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
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            if (ImGui::SliderFloat3("Look At", target_array, -10.0f, 10.0f)) {
                camera.set_look_at(point3(target_array[0], target_array[1], target_array[2]));
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
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
                camera.set_image_width(480);
                if (isCameraSpace) {
                    camera.transform_scene_and_lights(world, lights);
                }
            }
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
                camera.toggle_shadows();
            }

            if (ImGui::Checkbox("Toggle Wireframe", &renderWireframe)) {

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
static std::optional<ObjectID> selectedObjectID = std::nullopt;

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
            world.remove(selectedObjectID.value());
            selectedObjectID.reset();
        }
    }

    // Get the position and size of the main window to anchor the properties window
    ImVec2 mainWindowPos = ImGui::GetWindowPos();
    ImVec2 mainWindowSize = ImGui::GetWindowSize();

    // End the main window
    ImGui::End();

    // Set the position of the properties window to be below the main window
    ImGui::SetNextWindowPos(
        ImVec2(mainWindowPos.x, mainWindowPos.y + mainWindowSize.y),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(
        ImVec2(mainWindowSize.x, 0), // Width matches main window, height auto-adjusts
        ImGuiCond_Always
    );

    // Show the properties window
    ShowInfoWindow(world);
}

void ShowInfoWindow(HittableManager& world) {
    ImGui::Begin("Object Properties");
    if (ImGui::BeginTabBar("NodeWindowTabBar")) {
        // ----- Info Tab -----
        if (ImGui::BeginTabItem("Info")) {
            if (selectedObjectID.has_value()) {
                auto obj = world.get(selectedObjectID.value());
                if (obj) {
                    ImGui::Text("Object ID: %zu", selectedObjectID.value());
                    ImGui::Text("Object Type: %s", obj->get_type_name().c_str());

                    // Static variable to store octree depth
                    static int octreeDepth = 3;
                    ImGui::SliderInt("Octree Depth", &octreeDepth, 1, 6);

                    // Button to generate an octree for the selected object
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

                    // Check if an octree exists for the selected object
                    if (world.hasOctree(selectedObjectID.value())) {
                        ImGui::Text("Octree Generated!");

                        // Static variables to store calculated values
                        static double octreeVolume = 0.0;
                        static double octreeSurfaceArea = 0.0;

                        // Button to calculate and display octree volume
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

                        // Button to calculate and display octree surface area
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
                    }
                    else {
                        ImGui::Text("No Octree Generated");
                    }

                    // Check if the object is a CSGNode or CSGPrimitive
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
            ImGui::EndTabItem();
        }

        // ----- Geometry Tab (Transformations) -----
        if (ImGui::BeginTabItem("Geometry")) {
            if (selectedObjectID.has_value()) {
                auto obj = world.get(selectedObjectID.value());
                if (obj) {
                    // Get the bounding box center of the object
                    point3 center = obj->bounding_box().getCenter();

                    // Static variables to store translation values
                    static float translation[3] = { 0.0f, 0.0f, 0.0f };

                    // Display the current center of the object
                    ImGui::Text("Object Center: (%.2f, %.2f, %.2f)", center.x(), center.y(), center.z());

                    ImGui::Separator();

                    // Slider for translation input
                    ImGui::Text("Translate");

                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);

                    if (ImGui::SliderFloat3(" ", translation, -2.0f, 2.0f)) {
                    }

                    // Reset the item width to default
                    ImGui::PopItemWidth();

                    // Translate button
                    if (ImGui::Button("Transform")) {
                        // Calculate the new position relative to the center
                        point3 newPosition = center + vec3(translation[0], translation[1], translation[2]);

                        // Create a translation matrix
                        Matrix4x4 transform = Matrix4x4::translation(vec3(
                            translation[0], translation[1], translation[2]
                        ));

                        // Apply the transformation to the object
                        world.transform_object(selectedObjectID.value(), transform);

                        // Reset the translation values after applying the transformation
                        translation[0] = 0.0f;
                        translation[1] = 0.0f;
                        translation[2] = 0.0f;
                    }

                    // Reset button
                    ImGui::SameLine(); // Place the button next to the Translate button
                    if (ImGui::Button("Reset")) {
                        point3 targetPosition = point3(0.0f, 0.0f, -1.0f);

                        // Calculate the translation required to move the object to the target position
                        vec3 resetTranslation = targetPosition - center;

                        // Create a translation matrix for the reset
                        Matrix4x4 resetTransform = Matrix4x4::translation(resetTranslation);

                        world.transform_object(selectedObjectID.value(), resetTransform);

                        // Reset the translation values
                        translation[0] = 0.0f;
                        translation[1] = 0.0f;
                        translation[2] = 0.0f;
                    }
                }
            }
            else {
                ImGui::Text("Select an object to transform it.");
            }
            ImGui::EndTabItem();
        }

        // ----- Primitives Tab -----
        if (ImGui::BeginTabItem("Add Objects")) {
            if (ImGui::BeginTabBar("PrimitiveTabs")) {
                // Box Tab
                if (ImGui::BeginTabItem("Box")) {
                    static bool useMinMax = false; // Toggle between Center/Size and Vmin/Vmax

                    // Parameters for Center/Size method
                    static float boxCenter[3] = { 0.0f, 0.0f, -1.0f };
                    static float boxSize = 0.9f;

                    // Parameters for Vmin/Vmax method
                    static float vmin[3] = { 0.0f, 0.0f, 0.0f };
                    static float vmax[3] = { 0.9f, 0.9f, -0.9f };

                    static float boxColor[3] = { 1.0f, 0.0f, 0.0f };

                    ImGui::Checkbox("Use Vmin/Vmax", &useMinMax);

                    if (useMinMax) {
                        ImGui::SliderFloat3("Vmin", vmin, -2.0f, 2.0f);
                        ImGui::SliderFloat3("Vmax", vmax, -2.0f, 2.0f);
                    }
                    else {
                        ImGui::SliderFloat3("Center", boxCenter, -2.0f, 2.0f);
                        ImGui::SliderFloat("Size", &boxSize, 0.1f, 5.0f);
                    }

                    ImGui::ColorEdit3("Color", boxColor);

                    if (ImGui::Button("Create Box")) {
                        std::shared_ptr<CSGPrimitive> boxPrim;

                        if (useMinMax) {
                            // Create using vmin and vmax
                            boxPrim = std::make_shared<CSGPrimitive>(
                                std::make_shared<box_csg>(
                                    point3(vmin[0], vmin[1], vmin[2]),
                                    point3(vmax[0], vmax[1], vmax[2]),
                                    mat(color(boxColor[0], boxColor[1], boxColor[2]))
                                )
                            );
                        }
                        else {
                            // Create using center and width
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
                        // Reset values
                        boxCenter[0] = boxCenter[1] = 0.0f; boxCenter[2] = -1.0f;
                        boxSize = 0.9f;
                        vmin[0] = vmin[1] = -0.5f; vmin[2] = -1.5f;
                        vmax[0] = vmax[1] = 0.5f; vmax[2] = -0.5f;
                        boxColor[0] = 1.0f; boxColor[1] = 0.0f; boxColor[2] = 0.0f;
                    }

                    ImGui::EndTabItem();
                }

                // Sphere Tab
                if (ImGui::BeginTabItem("Sphere")) {
                    static float sphereCenter[3] = { 0.0f, 0.0f, -1.0f };
                    static float initialSphereCenter[3] = { 0.0f, 0.0f, -1.0f }; // Store initial values
                    static float sphereRadius = 0.6f;
                    static float initialSphereRadius = 0.6f; // Store initial size
                    static float sphereColor[3] = { 0.0f, 0.0f, 1.0f };
                    static float initialSphereColor[3] = { 0.0f, 0.0f, 1.0f }; // Store initial color

                    ImGui::SliderFloat3("Center", sphereCenter, -2.0f, 2.0f);
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

                    ImGui::SameLine(); // Place the Reset button next to the Create button
                    if (ImGui::Button("Reset")) {
                        // Reset values to initial ones
                        memcpy(sphereCenter, initialSphereCenter, sizeof(sphereCenter));
                        sphereRadius = initialSphereRadius;
                        memcpy(sphereColor, initialSphereColor, sizeof(sphereColor));
                    }

                    ImGui::EndTabItem();
                }

                // Cylinder Tab
                if (ImGui::BeginTabItem("Cylinder")) {
                    // Cylinder parameters
                    static float baseCenter[3] = { 0.0f, 0.0f, -1.0f };
                    static float topCenter[3] = { 0.0f, 1.0f, -1.0f };
                    static float radius = 0.5f;
                    static bool capped = true;
                    static float cylinderColor[3] = { 0.0f, 0.5f, 1.0f };

                    // Input controls
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
                        // Reset parameters to default values
                        baseCenter[0] = baseCenter[1] = 0.0f; baseCenter[2] = -1.0f;
                        topCenter[0] = topCenter[1] = 1.0f; topCenter[2] = -1.0f;
                        radius = 0.5f;
                        capped = true;
                        cylinderColor[0] = 0.0f; cylinderColor[1] = 0.5f; cylinderColor[2] = 1.0f;
                    }

                    ImGui::EndTabItem();
                }

                // Cone Tab
                if (ImGui::BeginTabItem("Cone")) {
                    // Cone parameters
                    static float baseCenter[3] = { 0.0f, -0.5f, -1.0f };
                    static float topVertex[3] = { 0.0f, 1.0f, -1.0f };
                    static float radius = 0.5f;
                    static bool capped = true;
                    static float coneColor[3] = { 1.0f, 0.5f, 0.0f };

                    // Input controls
                    ImGui::SliderFloat3("Base Center", baseCenter, -2.0f, 2.0f);
                    ImGui::SliderFloat3("Top Vertex", topVertex, -2.0f, 2.0f);
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
                        // Reset parameters to default values
                        baseCenter[0] = 0.0f; baseCenter[1] = -0.5f; baseCenter[2] = -1.0f;
                        topVertex[0] = 0.0f; topVertex[1] = 1.0f; topVertex[2] = -1.0f;
                        radius = 0.5f;
                        capped = true;
                        coneColor[0] = 1.0f; coneColor[1] = 0.5f; coneColor[2] = 0.0f;
                    }

                    ImGui::EndTabItem();
                }

                // Square Pyramid Tab
                if (ImGui::BeginTabItem("Square Pyramid")) {
                    // Pyramid parameters
                    static float baseCenter[3] = { 0.0f, -0.5f, -1.0f }; // Base center (inferiorPoint)
                    static float height = 1.0f;
                    static float baseSize = 1.0f;
                    static float pyramidColor[3] = { 0.8f, 0.3f, 0.0f };

                    // Input controls
                    ImGui::SliderFloat3("Base Center", baseCenter, -2.0f, 2.0f);
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
                        // Reset parameters to default values
                        baseCenter[0] = 0.0f; baseCenter[1] = -0.5f; baseCenter[2] = -1.0f;
                        height = 1.0f;
                        baseSize = 1.0f;
                        pyramidColor[0] = 0.8f; pyramidColor[1] = 0.3f; pyramidColor[2] = 0.0f;
                    }

                    ImGui::EndTabItem();
                }


                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }

        // ----- Boolean Tab (CSG Operations) -----
        if (ImGui::BeginTabItem("Boolean")) {
            // Static selection variables for the two objects and the chosen operation.
            static std::optional<ObjectID> leftObjectID = std::nullopt;
            static std::optional<ObjectID> rightObjectID = std::nullopt;
            static CSGType selectedOperation = CSGType::UNION;

            // Retrieve the current list of public objects.
            auto objectList = world.list_object_names();

            // Prepare display strings based on the current selections.
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

            // First object selection combo.
            ImGui::Text("Select First Object:");
            if (ImGui::BeginCombo("##FirstObject", leftDisplay.c_str())) {
                for (const auto& [id, name] : objectList) {
                    auto obj = world.get(id);
                    if (!obj) continue;

                    // Filter: Only show objects that are either CSGNode or CSGPrimitive
                    if (!dynamic_cast<CSGNode<Union>*>(obj.get()) &&
                        !dynamic_cast<CSGNode<Intersection>*>(obj.get()) &&
                        !dynamic_cast<CSGNode<Difference>*>(obj.get()) &&
                        !dynamic_cast<CSGPrimitive*>(obj.get())) {
                        continue;
                    }

                    bool isSelected = (leftObjectID.has_value() && leftObjectID.value() == id);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        leftObjectID = id;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Second object selection combo.
            ImGui::Text("Select Second Object:");
            if (ImGui::BeginCombo("##SecondObject", rightDisplay.c_str())) {
                for (const auto& [id, name] : objectList) {
                    auto obj = world.get(id);
                    if (!obj) continue;

                    // Filter: Only show objects that are either CSGNode or CSGPrimitive
                    if (!dynamic_cast<CSGNode<Union>*>(obj.get()) &&
                        !dynamic_cast<CSGNode<Intersection>*>(obj.get()) &&
                        !dynamic_cast<CSGNode<Difference>*>(obj.get()) &&
                        !dynamic_cast<CSGPrimitive*>(obj.get())) {
                        continue;
                    }

                    bool isSelected = (rightObjectID.has_value() && rightObjectID.value() == id);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        rightObjectID = id;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Dropdown for selecting the Boolean operation.
            ImGui::Text("Select Operation:");
            if (ImGui::BeginCombo("##Operation", csg_type_to_string(selectedOperation).c_str())) {
                if (ImGui::Selectable("Union", selectedOperation == CSGType::UNION)) {
                    selectedOperation = CSGType::UNION;
                }
                if (ImGui::Selectable("Intersection", selectedOperation == CSGType::INTERSECTION)) {
                    selectedOperation = CSGType::INTERSECTION;
                }
                if (ImGui::Selectable("Difference", selectedOperation == CSGType::DIFFERENCE)) {
                    selectedOperation = CSGType::DIFFERENCE;
                }
                ImGui::EndCombo();
            }

            // Button to apply the Boolean operation.
            if (ImGui::Button("Apply Boolean Operation")) {
                if (leftObjectID.has_value() && rightObjectID.has_value()) {
                    // Retrieve the objects from the world.
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
                            // Before removal, check if the global selectedObjectID refers to one of the soon-to-be removed primitives.
                            bool resetGlobalSelection = false;
                            if (selectedObjectID.has_value() &&
                                (selectedObjectID.value() == leftObjectID.value() ||
                                    selectedObjectID.value() == rightObjectID.value())) {
                                resetGlobalSelection = true;
                            }

                            // Add the composite CSG node to the world.
                            ObjectID newID = world.add(csgNode);
                            selectedObjectID = newID;

                            // Remove the absorbed primitive objects from the manager.
                            world.remove(leftObjectID.value());
                            world.remove(rightObjectID.value());

                            // Reset the selections for the boolean operation.
                            leftObjectID.reset();
                            rightObjectID.reset();
                        }
                    }
                }
            }

            ImGui::EndTabItem();
        }


        ImGui::EndTabBar();
    }
    ImGui::End();
}