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
    int image_width,
    int image_height) {
    vec3 pc = p - camera.origin;

    // Define a small epsilon to prevent division by zero
    const double epsilon = 1e-6;

    // Check if z is too close to zero
    if (std::abs(pc.e[2]) < epsilon) {
        pc.e[2] = (pc.e[2] < 0 ? -epsilon : epsilon);
    }

    // Clip points that are behind the camera
    if (pc.e[2] >= 0) {
        return std::nullopt; // No projection possible
    }

    // Calculate perspective scaling factor (Y-up, -Z-forward)
    double t = camera.focal_length / (-pc.e[2]);

    // Project the point onto the image plane
    vec3 plane_point = pc * t;

    // Normalize to viewport coordinates [0, 1]
    double u = (plane_point.x() + (camera.horizontal.length() / 2.0)) / camera.horizontal.length();
    double v = (plane_point.y() + (camera.vertical.length() / 2.0)) / camera.vertical.length();

    // Clamp u and v to [0, 1] to prevent out-of-bounds
    u = std::clamp(u, 0.0, 1.0);
    v = std::clamp(v, 0.0, 1.0);

    // Convert normalized coordinates to pixel coordinates
    int pixel_x = static_cast<int>(u * (image_width - 1));
    int pixel_y = static_cast<int>((1.0 - v) * (image_height - 1)); // Flip Y-axis for screen space

    return std::make_pair(pixel_x, pixel_y);
}

// Function to draw octree voxels
void DrawOctreeWireframe(SDL_Renderer* renderer,
    SDL_Window* window,
    const BoundingBox& root_bb,
    const std::vector<BoundingBox>& boxes,
    const Camera& camera, // Use Camera directly
    int image_width,
    int image_height)
{
    // Set up semi-transparent red lines for the wireframe
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);

    // Get current window size for scaling
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    float scale_x = static_cast<float>(window_w) / static_cast<float>(image_width);
    float scale_y = static_cast<float>(window_h) / static_cast<float>(image_height);

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
        {4,5}, {5,6}, {6,7}, {7,4}, // top face
        {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
    };

    // Function to draw a single bounding box
    auto draw_bounding_box = [&](const BoundingBox& bb) {
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
            auto p1 = project(corners[edge[0]], camera, image_width, image_height);
            auto p2 = project(corners[edge[1]], camera, image_width, image_height);

            // Skip edges where both points are behind the camera
            if (!p1 || !p2) {
                continue;
            }

            // Extract the scaled coordinates
            int scaled_x1 = static_cast<int>(p1->first * scale_x);
            int scaled_y1 = static_cast<int>(p1->second * scale_y);
            int scaled_x2 = static_cast<int>(p2->first * scale_x);
            int scaled_y2 = static_cast<int>(p2->second * scale_y);

            // Draw the line
            SDL_RenderDrawLine(renderer, scaled_x1, scaled_y1, scaled_x2, scaled_y2);
        }
        };

    // Draw the root bounding box first
    draw_bounding_box(root_bb);

    // Draw the remaining bounding boxes
    for (const auto& bb : boxes) {
        draw_bounding_box(bb);
    }
}

void DrawOctreeManagerWireframe(SDL_Renderer* renderer,
    SDL_Window* window,
    const OctreeManager& manager,
    const Camera& camera,
    int image_width,
    int image_height) {
    // Get current window size for scaling
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    float scale_x = static_cast<float>(window_w) / static_cast<float>(image_width);
    float scale_y = static_cast<float>(window_h) / static_cast<float>(image_height);

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
        {4,5}, {5,6}, {6,7}, {7,4}, // top face
        {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
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
            auto p1 = project(corners[edge[0]], camera, image_width, image_height);
            auto p2 = project(corners[edge[1]], camera, image_width, image_height);

            // Skip edges where both points are behind the camera
            if (!p1 || !p2) {
                continue;
            }

            // Extract the scaled coordinates
            int scaled_x1 = static_cast<int>(p1->first * scale_x);
            int scaled_y1 = static_cast<int>(p1->second * scale_y);
            int scaled_x2 = static_cast<int>(p2->first * scale_x);
            int scaled_y2 = static_cast<int>(p2->second * scale_y);

            // Draw the line
            SDL_RenderDrawLine(renderer, scaled_x1, scaled_y1, scaled_x2, scaled_y2);
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
