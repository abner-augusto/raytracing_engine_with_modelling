#pragma once
#include <SDL.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "boundingbox.h"
#include "camera.h"
#include "scene.h"

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
    const SceneManager& manager,
    const Camera& camera,
    const SDL_Rect& viewport,
    const std::optional<BoundingBox>& highlighted_box) {

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},  // bottom face
        {4, 5}, {5, 7}, {7, 6}, {6, 4},  // top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // vertical edges
    };

    auto draw_bounding_box = [&](const BoundingBox& bb, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        std::vector<point3> corners = bb.getVertices();
        for (const auto& edge : edges) {
            auto p1 = project(corners[edge[0]], camera, viewport);
            auto p2 = project(corners[edge[1]], camera, viewport);
            if (!p1 || !p2) continue;
            SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
        }
        };

    // Draw all objects' bounding boxes in green
    auto objects = manager.getObjects();
    for (const auto& object : objects) {
        draw_bounding_box(object->bounding_box(), { 0, 255, 0, 64 });
    }

    // Draw octree bounding boxes in red
    auto octree_boxes = manager.getAllOctreeFilledBoundingBoxes();
    for (const auto& box : octree_boxes) {
        draw_bounding_box(box, { 255, 0, 0, 255 });
    }

    // Draw highlighted bounding box in blue (if any)
    if (highlighted_box) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        draw_bounding_box(*highlighted_box, { 0, 0, 255, 255 });
    }
}

void render_world_axes(SDL_Renderer* renderer, const Camera& camera, const SDL_Rect& viewport) {
    
    double axis_length = 1.5;
    double arrow_size = 0.3;

    // Define axis endpoints in world space
    point3 origin(0, 0, 0);
    point3 x_end(axis_length, 0, 0);
    point3 y_end(0, axis_length, 0);
    point3 z_end(0, 0, axis_length);

    // Compute arrowhead points in world space
    point3 x_arrow1(axis_length - arrow_size, arrow_size / 2, 0);
    point3 x_arrow2(axis_length - arrow_size, -arrow_size / 2, 0);

    point3 y_arrow1(arrow_size / 2, axis_length - arrow_size, 0);
    point3 y_arrow2(-arrow_size / 2, axis_length - arrow_size, 0);

    point3 z_arrow1(0, arrow_size / 2, axis_length - arrow_size);
    point3 z_arrow2(0, -arrow_size / 2, axis_length - arrow_size);

    // Project the origin, axes, and arrow points to screen space
    auto origin_proj = project(origin, camera, viewport);
    auto x_proj = project(x_end, camera, viewport);
    auto y_proj = project(y_end, camera, viewport);
    auto z_proj = project(z_end, camera, viewport);

    auto x_arrow1_proj = project(x_arrow1, camera, viewport);
    auto x_arrow2_proj = project(x_arrow2, camera, viewport);

    auto y_arrow1_proj = project(y_arrow1, camera, viewport);
    auto y_arrow2_proj = project(y_arrow2, camera, viewport);

    auto z_arrow1_proj = project(z_arrow1, camera, viewport);
    auto z_arrow2_proj = project(z_arrow2, camera, viewport);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Draw the X-axis in red
    if (origin_proj && x_proj) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, origin_proj->first, origin_proj->second, x_proj->first, x_proj->second);
        if (x_arrow1_proj && x_arrow2_proj) {
            SDL_RenderDrawLine(renderer, x_proj->first, x_proj->second, x_arrow1_proj->first, x_arrow1_proj->second);
            SDL_RenderDrawLine(renderer, x_proj->first, x_proj->second, x_arrow2_proj->first, x_arrow2_proj->second);
        }
    }

    // Draw the Y-axis in green
    if (origin_proj && y_proj) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawLine(renderer, origin_proj->first, origin_proj->second, y_proj->first, y_proj->second);
        if (y_arrow1_proj && y_arrow2_proj) {
            SDL_RenderDrawLine(renderer, y_proj->first, y_proj->second, y_arrow1_proj->first, y_arrow1_proj->second);
            SDL_RenderDrawLine(renderer, y_proj->first, y_proj->second, y_arrow2_proj->first, y_arrow2_proj->second);
        }
    }

    // Draw the Z-axis in blue
    if (origin_proj && z_proj) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawLine(renderer, origin_proj->first, origin_proj->second, z_proj->first, z_proj->second);
        if (z_arrow1_proj && z_arrow2_proj) {
            SDL_RenderDrawLine(renderer, z_proj->first, z_proj->second, z_arrow1_proj->first, z_arrow1_proj->second);
            SDL_RenderDrawLine(renderer, z_proj->first, z_proj->second, z_arrow2_proj->first, z_arrow2_proj->second);
        }
    }

    // Reset color
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}
