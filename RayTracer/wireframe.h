#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "boundingbox.h"
#include "camera.h"
#include "hittable_manager.h"

// Project a 3D point (in world space) to 2D screen space.
std::optional<std::pair<int, int>> project(const point3& p, const Camera& camera, const SDL_Rect& viewport) {
    // Transform the point from world space to camera space.
    // We use homogeneous coordinates: assume point3 can be extended to vec4 with w=1.
    vec4 p_world(p, 1.0);
    vec4 p_cam = camera.world_to_camera_matrix * p_world;

    // In camera space the camera looks along the negative Z axis.
    // If z is non-negative, the point is behind the camera.
    if (p_cam.z >= 0) {
        return std::nullopt;
    }

    // Compute perspective parameters.
    double fov_radians = degrees_to_radians(camera.get_fov_degrees());
    double tan_half_fov = std::tan(0.5 * fov_radians);

    // The aspect ratio is defined as image_width / image_height.
    double aspect = camera.get_image_width() / static_cast<double>(camera.get_image_height());

    // Perform the perspective division.
    // Compute normalized device coordinates (NDC) in the range [-1, 1].
    // Here we follow a pinhole camera model: 
    double x_ndc = (p_cam.x / -p_cam.z) / (tan_half_fov * aspect);
    double y_ndc = (p_cam.y / -p_cam.z) / tan_half_fov;

    // Convert NDC to screen (pixel) coordinates.
    // NDC (-1 to 1) is mapped to pixel space.
    int screen_x = static_cast<int>((x_ndc + 1.0) * 0.5 * viewport.w);
    int screen_y = static_cast<int>((1.0 - (y_ndc + 1.0) * 0.5) * viewport.h);

    return std::make_pair(screen_x, screen_y);
}

// Function to draw octree voxels
void DrawOctreeWireframe(SDL_Renderer* renderer,
    const HittableManager& manager,
    const Camera& camera,
    const SDL_Rect& viewport)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},  // bottom face
        {4, 5}, {5, 7}, {7, 6}, {6, 4},  // top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // vertical edges
    };

    auto draw_bounding_box = [&](const BoundingBox& bb) {
        std::vector<point3> corners = bb.getVertices();
        for (const auto& edge : edges) {
            auto p1 = project(corners[edge[0]], camera, viewport);
            auto p2 = project(corners[edge[1]], camera, viewport);
            if (!p1 || !p2) continue;
            SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
        }
        };

    // Get all objects from the manager
    auto objects = manager.getObjects();

    // Draw bounding boxes for all objects in the manager
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64);
    for (const auto& object : objects) {
        draw_bounding_box(object->bounding_box());
    }

    // Get all filled bounding boxes from octree
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    auto octree_boxes = manager.getAllOctreeFilledBoundingBoxes();
    for (const auto& box : octree_boxes) {
        draw_bounding_box(box);
    }
}

