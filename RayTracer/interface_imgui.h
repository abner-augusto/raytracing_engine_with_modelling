#pragma once

#include "raytracer.h"
#include "camera.h"
#include "octreemanager.h"
#include <vector>

void draw_menu(bool& render_raytracing,
    bool& show_wireframe,
    double& speed,
    Camera& camera,
    point3& origin,
    hittable_list& world);

void DrawFpsCounter(float fps);

// Octree Manager components
void RenderOctreeList(OctreeManager& manager);
void RenderOctreeInspector(OctreeManager& manager, hittable_list& world);
void RenderTreeRepresentationWindow(OctreeManager& manager, bool& show_window);