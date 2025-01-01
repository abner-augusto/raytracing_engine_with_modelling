#pragma once

#include "raytracer.h"
#include "camera.h"
#include "octreemanager.h"
#include "render_state.h"
#include <vector>
#include <imgui.h>

// ---------------------------------------------------------------------------
// Main "menu" and global UI
// ---------------------------------------------------------------------------

void draw_menu(
    RenderState& render_state,
    bool& show_wireframe,
    double& speed,
    Camera& camera,
    point3& origin,
    hittable_list& world
);

void DrawFpsCounter(float fps);

// ---------------------------------------------------------------------------
// Octree Manager UI
// ---------------------------------------------------------------------------

void RenderOctreeList(OctreeManager& manager);
void RenderOctreeInspector(OctreeManager& manager, hittable_list& world);
void RenderBooleanOperations(OctreeManager& manager);
int InputTextCallback_Resize(ImGuiInputTextCallbackData* data);
void RenderRebuildOctreeUI(OctreeManager& manager);
void RenderInspectorTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void RenderPrimitivesTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void RenderPrimitivesSphereTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void RenderRenderTab(OctreeManager& manager, int selected_index, hittable_list& world, OctreeManager::OctreeWrapper& wrapper);
void RenderIOTab(OctreeManager& manager, int selected_index);
void RenderPrimitivesCubeTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void RenderPrimitivesCylinderTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void RenderPrimitivesPyramidTab(OctreeManager& manager, int selected_index, BoundingBox& bb);
void GenerateOctreeFromSquarePyramid(OctreeManager& manager, int selected_index, BoundingBox& bb, float pyramid_height, float pyramid_basis, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string);
void GenerateOctreeFromSphere(OctreeManager& manager, int selected_index, BoundingBox& bb, float sphere_radius, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string);
void GenerateOctreeFromCylinder(OctreeManager& manager, int selected_index, BoundingBox& bb, double height, double radius, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string);
void GenerateOctreeFromBox(OctreeManager& manager, int selected_index, BoundingBox& bb, float box_width, bool centered, point3 custom_center, int depth_limit, std::string& octree_string, bool& show_octree_string);