#pragma once

#include <vector>
#include <imgui.h>
#include <cmath>
#include <chrono>

#include "camera.h"
#include "matrix4x4.h"
#include "render_state.h"
#include "csg.h"
#include "scene.h"
#include "sphere.h"
#include "box_csg.h"
#include "cylinder.h"
#include "cone.h"
#include "squarepyramid.h"
#include "light.h"
#include "mesh.h"
#include "dialog_box.h"
#include "scene_builder.h"

void draw_menu(RenderState& render_state, Camera& camera, SceneManager& world, SceneBuilder& builder);

void DrawFpsCounter(float fps);

void ShowHittableManagerUI(SceneManager& world, Camera& camera);

void ShowLightsUI(SceneManager& world);

// Displays the main ImGui window for object properties.
void ShowInfoWindow(SceneManager& world);

// Displays the "Info" tab where object details and octree controls are shown.
void ShowInfoTab(SceneManager& world);

// Displays the "Geometry" tab for object transformations.
void ShowGeometryTab(SceneManager& world);

// Displays the "Primitives" tab with sub-tabs for creating different primitives.
void ShowPrimitivesTab(SceneManager& world);

// Displays the "Boolean" tab for applying CSG operations between objects.
void ShowBooleanTab(SceneManager& world);

extern std::optional<ObjectID> selectedObjectID;

extern std::optional<BoundingBox> highlighted_box;