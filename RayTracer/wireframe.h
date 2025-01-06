#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "boundingbox.h"
#include "vec3.h"
#include "camera.h"
#include "octreemanager.h"

std::optional<std::pair<int, int>> project(const point3& p,
    const Camera& camera,
    const SDL_Rect& viewport) {

    // Transform point to camera space
    point3 camera_space = camera.get_world_to_camera_matrix().transform_point(p);

    const double epsilon = 1e-6;

    // Check if point is too close to image plane
    if (std::abs(camera_space.z()) < epsilon) {
        camera_space.e[2] = (camera_space.z() < 0 ? -epsilon : epsilon);
    }

    // Points in front of the camera have negative Z in camera space
    if (camera_space.z() >= 0) {
        return std::nullopt;
    }

    double focal_length = camera.get_focal_length();

    // Project to image plane (perspective division)
    double x_proj = -focal_length * camera_space.x() / camera_space.z();
    double y_proj = -focal_length * camera_space.y() / camera_space.z();

    // Convert to NDC space (0 to 1)
    double viewport_width = camera.get_viewport_width();
    double viewport_height = camera.get_viewport_height();

    double norm_u = (x_proj + viewport_width / 2.0) / viewport_width;
    double norm_v = (y_proj + viewport_height / 2.0) / viewport_height;

    // Convert to screen space (with Y-flip) using SDL_Rect bounds
    int pixel_x = static_cast<int>(viewport.x + norm_u * viewport.w);
    int pixel_y = static_cast<int>(viewport.y + (1.0 - norm_v) * viewport.h);

    // Check bounds
    if (pixel_x >= viewport.x && pixel_x < (viewport.x + viewport.w) &&
        pixel_y >= viewport.y && pixel_y < (viewport.y + viewport.h)) {
        return std::make_pair(pixel_x, pixel_y);
    }

    return std::nullopt;
}

// Function to draw octree voxels
void DrawOctreeWireframe(SDL_Renderer* renderer,
    const BoundingBox& root_bb,
    const std::vector<BoundingBox>& boxes,
    const Camera& camera,
    const SDL_Rect& viewport)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128); // Green for bounding boxes

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
    };

    auto draw_bounding_box = [&](const BoundingBox& bb) {
        point3 vmin = bb.vmin;
        point3 vmax = bb.vmax();

        point3 corners[8] = {
            point3(vmin.x(), vmin.y(), vmin.z()),
            point3(vmax.x(), vmin.y(), vmin.z()),
            point3(vmax.x(), vmax.y(), vmin.z()),
            point3(vmin.x(), vmax.y(), vmin.z()),
            point3(vmin.x(), vmin.y(), vmax.z()),
            point3(vmax.x(), vmin.y(), vmax.z()),
            point3(vmax.x(), vmax.y(), vmax.z()),
            point3(vmin.x(), vmax.y(), vmax.z()),
        };

        for (auto& edge : edges) {
            auto p1 = project(corners[edge[0]], camera, viewport);
            auto p2 = project(corners[edge[1]], camera, viewport);

            if (!p1 || !p2) {
                continue;
            }

            SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
        }
        };

    // Draw the root bounding box and others
    draw_bounding_box(root_bb);
    for (const auto& bb : boxes) {
        draw_bounding_box(bb);
    }
}

void DrawOctreeManagerWireframe(SDL_Renderer* renderer,
    const OctreeManager& manager,
    const Camera& camera,
    const SDL_Rect& viewport)
{
    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
    };

    // Function to draw a single bounding box with a specific color
    auto draw_bounding_box = [&](const BoundingBox& bb, SDL_Color color) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        point3 vmin = bb.vmin;
        point3 vmax = bb.vmax();

        // Define the 8 corners of the bounding box
        point3 corners[8] = {
            point3(vmin.x(), vmin.y(), vmin.z()),
            point3(vmax.x(), vmin.y(), vmin.z()),
            point3(vmax.x(), vmax.y(), vmin.z()),
            point3(vmin.x(), vmax.y(), vmin.z()),
            point3(vmin.x(), vmin.y(), vmax.z()),
            point3(vmax.x(), vmin.y(), vmax.z()),
            point3(vmax.x(), vmax.y(), vmax.z()),
            point3(vmin.x(), vmax.y(), vmax.z()),
        };

        // Project and draw each edge
        for (auto& edge : edges) {
            auto p1 = project(corners[edge[0]], camera, viewport);
            auto p2 = project(corners[edge[1]], camera, viewport);

            // Skip edges where both points are behind the camera
            if (!p1 || !p2) {
                continue;
            }

            // Draw the line
            SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
        }
        };

    // Iterate over each octree in the manager and render its bounding boxes
    for (const auto& wrapper : manager.GetOctrees()) {
        const Octree& octree = *wrapper.octree;

        // Draw the root bounding box in green
        draw_bounding_box(octree.bounding_box, { 0, 255, 0, 128 });

        // Draw the filled bounding boxes in red
        auto filled_boxes = octree.GetFilledBoundingBoxes();
        for (const auto& bb : filled_boxes) {
            draw_bounding_box(bb, { 255, 0, 0, 128 });
        }
    }
}
