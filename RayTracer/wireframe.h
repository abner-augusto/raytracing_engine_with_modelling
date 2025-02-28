#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "boundingbox.h"
#include "camera.h"
#include "scene.h"
#include "winged_edge.h"
#include "winged_edge_ui.h"

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
    
    double axis_length = 0.5;
    double arrow_size = 0.1;

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

void RenderWingedEdgeWireframe(SDL_Renderer* renderer,
    const MeshCollection& meshCollection,
    const Camera& camera,
    const SDL_Rect& viewport,
    WingedEdgeImGui& imguiInterface,
    bool renderArrow = true) {

    // Get the vector of selected edges (if an edge loop is active)
    const auto& selectedEdges = imguiInterface.getSelectedEdgeLoop();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    for (const auto& meshPtr : meshCollection) {
        if (!meshPtr) continue;
        const WingedEdge& mesh = *meshPtr;

        for (const auto& edgePtr : mesh.edges) {
            if (!edgePtr) continue;

            auto p1 = project(edgePtr->origVec, camera, viewport);
            auto p2 = project(edgePtr->destVec, camera, viewport);

            if (p1 && p2) {
                // Set color and draw the edge
                if (!selectedEdges.empty() &&
                    std::find(selectedEdges.begin(), selectedEdges.end(), edgePtr) != selectedEdges.end()) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for selected loop edges

                    // Draw a thicker line by offsetting perpendicular directions
                    float dx = static_cast<float>(p2->first - p1->first);
                    float dy = static_cast<float>(p2->second - p1->second);
                    float length = sqrt(dx * dx + dy * dy);
                    if (length > 0) {
                        float offsetX = (-dy / length);
                        float offsetY = (dx / length);

                        SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
                        SDL_RenderDrawLine(renderer,
                            static_cast<int>(p1->first + offsetX), static_cast<int>(p1->second + offsetY),
                            static_cast<int>(p2->first + offsetX), static_cast<int>(p2->second + offsetY));
                        SDL_RenderDrawLine(renderer,
                            static_cast<int>(p1->first - offsetX), static_cast<int>(p1->second - offsetY),
                            static_cast<int>(p2->first - offsetX), static_cast<int>(p2->second - offsetY));
                    }
                }
                else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White for normal edges
                    SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
                }

                // --- Arrow drawing code ---
                if (renderArrow) { 
                    // Choose a point along the edge for the arrow tip
                    float arrowFactor = 0.75f;
                    int tipX = p1->first + static_cast<int>(arrowFactor * (p2->first - p1->first));
                    int tipY = p1->second + static_cast<int>(arrowFactor * (p2->second - p1->second));

                    // Compute the unit direction vector along the edge
                    float dx = static_cast<float>(p2->first - p1->first);
                    float dy = static_cast<float>(p2->second - p1->second);
                    float len = sqrt(dx * dx + dy * dy);
                    if (len > 0) {
                        float udx = dx / len;
                        float udy = dy / len;

                        // Compute a perpendicular vector
                        float perpX = -udy;
                        float perpY = udx;

                        // Parameters for the arrowhead (length and angle)
                        float arrowHeadLength = 10.0f; // pixels
                        float arrowHeadAngle = pi / 6; // 30 degrees in radians

                        float cosAngle = cos(arrowHeadAngle);
                        float sinAngle = sin(arrowHeadAngle);

                        // Calculate the endpoints for the arrow wings
                        float leftWingX = tipX - arrowHeadLength * (udx * cosAngle + perpX * sinAngle);
                        float leftWingY = tipY - arrowHeadLength * (udy * cosAngle + perpY * sinAngle);
                        float rightWingX = tipX - arrowHeadLength * (udx * cosAngle - perpX * sinAngle);
                        float rightWingY = tipY - arrowHeadLength * (udy * cosAngle - perpY * sinAngle);

                        // Draw the arrowhead lines
                        SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(leftWingX), static_cast<int>(leftWingY));
                        SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(rightWingX), static_cast<int>(rightWingY));
                    }
                }
            }
        }

        // Render vertices as usual
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (const auto& vertex : mesh.vertices) {
            auto projected = project(vertex, camera, viewport);
            if (projected) {
                SDL_Rect pointRect = { projected->first - 2, projected->second - 2, 9, 9 };
                SDL_RenderFillRect(renderer, &pointRect);
            }
        }
    }
}

void DrawCrosshair(SDL_Renderer* renderer, int window_width, int window_height) {
    int center_x = window_width / 2;
    int center_y = window_height / 2;
    int crosshair_size = 10; // Size of the crosshair

    // Set the draw color to white (or any color you prefer)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw horizontal line
    SDL_RenderDrawLine(renderer, center_x - crosshair_size, center_y, center_x + crosshair_size, center_y);

    // Draw vertical line
    SDL_RenderDrawLine(renderer, center_x, center_y - crosshair_size, center_x, center_y + crosshair_size);
}
