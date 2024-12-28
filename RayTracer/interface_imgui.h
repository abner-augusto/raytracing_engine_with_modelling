#pragma once

#include "raytracer.h"
#include "camera.h"
#include "octreemanager.h"
#include "render_state.h"
#include <vector>
#include <imgui.h>

void draw_menu(RenderState& render_state,
    bool& show_wireframe,
    double& speed,
    Camera& camera,
    point3& origin,
    hittable_list& world);

void DrawFpsCounter(float fps);

// Octree Manager components
void RenderOctreeList(OctreeManager& manager);
void RenderOctreeInspector(OctreeManager& manager, hittable_list& world);
void RenderBooleanOperations(OctreeManager& manager);
void PerformBooleanOperation(OctreeManager& manager, int index1, int index2, int result_index, const std::string& operation);
void GenerateOctreeFromSphere(OctreeManager::OctreeWrapper& wrapper, BoundingBox& bb, float sphere_radius, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string);
int InputTextCallback_Resize(ImGuiInputTextCallbackData* data);