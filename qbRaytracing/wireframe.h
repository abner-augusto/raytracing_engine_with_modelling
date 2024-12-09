#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "boundingbox.h"
#include "vec3.h"

// origin, focal length (f), image_width, image_height, etc.
std::pair<int, int> project(const point3& p,
    const point3& origin,
    double focal_length,
    double viewport_width,
    double viewport_height,
    int image_width,
    int image_height)
{
    vec3 pc = p - origin;

    // Intersection with image plane at z = -focal_length
    double t = -focal_length / pc.z();
    vec3 plane_point = pc * t;

    double u = (plane_point.x() + viewport_width / 2.0) / viewport_width;
    double v = (plane_point.y() + viewport_height / 2.0) / viewport_height;

    // Flip the y-axis:
    int pixel_x = (int)(u * (image_width - 1));
    int pixel_y = (int)((1.0 - v) * (image_height - 1));

    return std::make_pair(pixel_x, pixel_y);
}

// Function to draw wireframe boxes
void draw_wireframe_bounding_boxes(SDL_Renderer* renderer,
    SDL_Window* window,
    const std::vector<BoundingBox>& boxes,
    const point3& origin,
    double focal_length,
    double viewport_width,
    double viewport_height,
    int image_width,
    int image_height)
{
    // Set up semi-transparent red lines for the wireframe
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);

    // Get current window size for scaling
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    float scale_x = (float)window_w / (float)image_width;
    float scale_y = (float)window_h / (float)image_height;

    // Define edges of a cube (using indices 0-7 for corners)
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
        {4,5}, {5,6}, {6,7}, {7,4}, // top face
        {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
    };

    for (const auto& bb : boxes) {
        // Compute all corners of the bounding box
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

        // Project and draw edges
        for (auto& edge : edges) {
            auto p1 = project(corners[edge[0]], origin, focal_length, viewport_width, viewport_height, image_width, image_height);
            auto p2 = project(corners[edge[1]], origin, focal_length, viewport_width, viewport_height, image_width, image_height);

            // Scale to current window size
            int scaled_x1 = (int)(p1.first * scale_x);
            int scaled_y1 = (int)(p1.second * scale_y);
            int scaled_x2 = (int)(p2.first * scale_x);
            int scaled_y2 = (int)(p2.second * scale_y);

            SDL_RenderDrawLine(renderer, scaled_x1, scaled_y1, scaled_x2, scaled_y2);
        }
    }
}
