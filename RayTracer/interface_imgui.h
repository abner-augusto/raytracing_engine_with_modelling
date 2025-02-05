#pragma once

#include <vector>
#include <imgui.h>
#include <cmath>
#include <chrono>

#include "camera.h"
#include "matrix4x4.h"
#include "render_state.h"
#include "csg.h"
#include "hittable_manager.h"
#include "sphere.h"
#include "box_csg.h"
#include "cylinder.h"
#include "cone.h"
#include "squarepyramid.h"

void draw_menu(RenderState& render_state,
               Camera& camera, 
               HittableManager world,
               std::vector<Light>& lights
              );

void DrawFpsCounter(float fps);

void ShowHittableManagerUI(HittableManager& world);

// Displays the main ImGui window for object properties.
void ShowInfoWindow(HittableManager& world);

// Displays the "Info" tab where object details and octree controls are shown.
void ShowInfoTab(HittableManager& world);

// Displays the "Geometry" tab for object transformations.
void ShowGeometryTab(HittableManager& world);

// Displays the "Primitives" tab with sub-tabs for creating different primitives.
void ShowPrimitivesTab(HittableManager& world);

// Displays the "Boolean" tab for applying CSG operations between objects.
void ShowBooleanTab(HittableManager& world);

extern std::optional<ObjectID> selectedObjectID;

extern std::optional<BoundingBox> highlighted_box;