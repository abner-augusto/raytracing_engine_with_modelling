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
    vec3 pc = p - camera.get_origin();

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
    double t = camera.get_focal_length() / (-pc.e[2]);

    // Project the point onto the image plane
    vec3 plane_point = pc * t;

    // Normalize to viewport coordinates [0, 1]
    double u = (plane_point.x() + (camera.get_horizontal_length() / 2.0)) / camera.get_horizontal_length();
    double v = (plane_point.y() + (camera.get_vertical_length() / 2.0)) / camera.get_vertical_length();

    // Clamp u and v to [0, 1] to prevent out-of-bounds
    u = std::clamp(u, 0.0, 1.0);
    v = std::clamp(v, 0.0, 1.0);

    // Convert normalized coordinates to pixel coordinates
    int pixel_x = static_cast<int>(u * (image_width - 1));
    int pixel_y = static_cast<int>((1.0 - v) * (image_height - 1)); // Flip Y-axis for screen space

    return std::make_pair(pixel_x, pixel_y);
}

// Utility function to clip a line to the destination rectangle
bool clip_line(int& x1, int& y1, int& x2, int& y2, const SDL_Rect& rect) {
    // Cohen-Sutherland line clipping algorithm
    auto compute_out_code = [](int x, int y, const SDL_Rect& rect) {
        int code = 0;
        if (x < rect.x) code |= 1;             // Left
        else if (x > rect.x + rect.w) code |= 2; // Right
        if (y < rect.y) code |= 4;             // Top
        else if (y > rect.y + rect.h) code |= 8; // Bottom
        return code;
        };

    int out_code1 = compute_out_code(x1, y1, rect);
    int out_code2 = compute_out_code(x2, y2, rect);

    while (true) {
        if (!(out_code1 | out_code2)) {
            // Both endpoints are inside the rectangle
            return true;
        }
        else if (out_code1 & out_code2) {
            // Both endpoints are outside the rectangle (in the same region)
            return false;
        }
        else {
            // At least one endpoint is outside the rectangle; clip the line
            int out_code_out = out_code1 ? out_code1 : out_code2;

            int x, y;

            if (out_code_out & 8) { // Point is below the rectangle
                x = x1 + (x2 - x1) * (rect.y + rect.h - y1) / (y2 - y1);
                y = rect.y + rect.h;
            }
            else if (out_code_out & 4) { // Point is above the rectangle
                x = x1 + (x2 - x1) * (rect.y - y1) / (y2 - y1);
                y = rect.y;
            }
            else if (out_code_out & 2) { // Point is to the right of the rectangle
                y = y1 + (y2 - y1) * (rect.x + rect.w - x1) / (x2 - x1);
                x = rect.x + rect.w;
            }
            else if (out_code_out & 1) { // Point is to the left of the rectangle
                y = y1 + (y2 - y1) * (rect.x - x1) / (x2 - x1);
                x = rect.x;
            }

            if (out_code_out == out_code1) {
                x1 = x;
                y1 = y;
                out_code1 = compute_out_code(x1, y1, rect);
            }
            else {
                x2 = x;
                y2 = y;
                out_code2 = compute_out_code(x2, y2, rect);
            }
        }
    }
}

// Function to draw octree voxels
void DrawOctreeWireframe(SDL_Renderer* renderer,
    const BoundingBox& root_bb,
    const std::vector<BoundingBox>& boxes,
    const Camera& camera,
    const SDL_Rect& destination_rect) // Use destination_rect for clipping
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128); // Green for bounding boxes

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
        {4,5}, {5,6}, {6,7}, {7,4}, // top face
        {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
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
            auto p1 = project(corners[edge[0]], camera, destination_rect.w, destination_rect.h);
            auto p2 = project(corners[edge[1]], camera, destination_rect.w, destination_rect.h);

            if (!p1 || !p2) {
                continue;
            }

            int x1 = destination_rect.x + p1->first;
            int y1 = destination_rect.y + p1->second;
            int x2 = destination_rect.x + p2->first;
            int y2 = destination_rect.y + p2->second;

            if (clip_line(x1, y1, x2, y2, destination_rect)) {
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
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
    const SDL_Rect& destination_rect)
{
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
            auto p1 = project(corners[edge[0]], camera, destination_rect.w, destination_rect.h);
            auto p2 = project(corners[edge[1]], camera, destination_rect.w, destination_rect.h);

            // Skip edges where both points are behind the camera
            if (!p1 || !p2) {
                continue;
            }

            // Extract the coordinates and scale them based on destination_rect
            int scaled_x1 = destination_rect.x + static_cast<int>(p1->first);
            int scaled_y1 = destination_rect.y + static_cast<int>(p1->second);
            int scaled_x2 = destination_rect.x + static_cast<int>(p2->first);
            int scaled_y2 = destination_rect.y + static_cast<int>(p2->second);

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
