#pragma once
#include <SDL2/SDL.h>
#include <SDL_ttf.h>
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
    TTF_Font* font,
    bool renderArrow = true,
    bool renderText = true) {

    const auto& selectedEdges = imguiInterface.getSelectedEdgeLoop();

    for (size_t i = 0; i < meshCollection.getMeshCount(); i++) {
        const WingedEdge* mesh = meshCollection.getMesh(i);
        if (!mesh) continue;

        // Determine which faces are front-facing (visible) versus back-facing.
        std::unordered_map<const Face*, bool> faceVisibility;
        for (const auto& face : mesh->faces) {
            vec3 faceNormal = face->normal();
            vec3 viewDir = camera.get_origin() - face->vertices[0]->pos;
            double dotProduct = dot(faceNormal, viewDir.normalized());
            faceVisibility[face.get()] = (dotProduct > 0);
        }

        // Render edges with transparency based on face visibility.
        for (const auto& edgePtr : mesh->edges) {
            if (!edgePtr) continue;

            auto p1 = project(edgePtr->origin->pos, camera, viewport);
            auto p2 = project(edgePtr->destination->pos, camera, viewport);

            if (p1 && p2) {
                Uint8 alpha = 255; // full opacity by default

                // If the edge belongs to any back-facing face, lower opacity.
                auto leftFace = edgePtr->leftFace.lock();
                auto rightFace = edgePtr->rightFace.lock();
                if ((leftFace && !faceVisibility[leftFace.get()]) &&
                    (rightFace && !faceVisibility[rightFace.get()])) {
                    alpha = 50;
                }

                // Determine if this edge is selected.
                bool isSelected = (!selectedEdges.empty() &&
                    std::find(selectedEdges.begin(), selectedEdges.end(), edgePtr) != selectedEdges.end());

                if (isSelected) {
                    // No blend mode for selected edges
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

                    // Create a thicker line by drawing multiple lines with small offsets
                    int lineThickness = 3; // Adjust for desired thickness

                    // Draw the main line
                    SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);

                    // Calculate perpendicular vector for offsets
                    float dx = static_cast<float>(p2->first - p1->first);
                    float dy = static_cast<float>(p2->second - p1->second);
                    float len = sqrt(dx * dx + dy * dy);

                    if (len > 0) { // Avoid division by zero
                        // Perpendicular unit vector
                        float perpX = -dy / len;
                        float perpY = dx / len;

                        // Draw offset lines to create thickness
                        for (int offset = 1; offset <= lineThickness; ++offset) {
                            // Upper offset line
                            int x1_offset = p1->first + static_cast<int>(perpX * offset);
                            int y1_offset = p1->second + static_cast<int>(perpY * offset);
                            int x2_offset = p2->first + static_cast<int>(perpX * offset);
                            int y2_offset = p2->second + static_cast<int>(perpY * offset);
                            SDL_RenderDrawLine(renderer, x1_offset, y1_offset, x2_offset, y2_offset);

                            // Lower offset line
                            x1_offset = p1->first - static_cast<int>(perpX * offset);
                            y1_offset = p1->second - static_cast<int>(perpY * offset);
                            x2_offset = p2->first - static_cast<int>(perpX * offset);
                            y2_offset = p2->second - static_cast<int>(perpY * offset);
                            SDL_RenderDrawLine(renderer, x1_offset, y1_offset, x2_offset, y2_offset);
                        }
                    }
                }
                else {
                    // Render non-selected edge:  blend if transparent, no-blend if opaque.
                    if (alpha == 255) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    }
                    else {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    }
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                    SDL_RenderDrawLine(renderer, p1->first, p1->second, p2->first, p2->second);
                }


                // Render arrowhead (if enabled).
                if (renderArrow) {
                    float arrowFactor = 0.75f;
                    int tipX = p1->first + static_cast<int>(arrowFactor * (p2->first - p1->first));
                    int tipY = p1->second + static_cast<int>(arrowFactor * (p2->second - p1->second));

                    float dx = static_cast<float>(p2->first - p1->first);
                    float dy = static_cast<float>(p2->second - p1->second);
                    float len = sqrt(dx * dx + dy * dy);
                    if (len > 0) { // avoid division by zero
                        float udx = dx / len;
                        float udy = dy / len;
                        float perpX = -udy;
                        float perpY = udx;
                        float arrowHeadLength = 10.0f;
                        float arrowHeadAngle = 3.14159f / 6;  // approximately 30 degrees

                        float cosAngle = cos(arrowHeadAngle);
                        float sinAngle = sin(arrowHeadAngle);

                        float leftWingX = tipX - arrowHeadLength * (udx * cosAngle + perpX * sinAngle);
                        float leftWingY = tipY - arrowHeadLength * (udy * cosAngle + perpY * sinAngle);
                        float rightWingX = tipX - arrowHeadLength * (udx * cosAngle - perpX * sinAngle);
                        float rightWingY = tipY - arrowHeadLength * (udy * cosAngle - perpY * sinAngle);

                        if (isSelected) {
                            // For selected edges, draw arrowheads in green and thicker.
                            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                            SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(leftWingX), static_cast<int>(leftWingY));
                            SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(rightWingX), static_cast<int>(rightWingY));

                            // Thicken the arrowheads using offsets (using same thickness as edge)
                            int arrowThickness = 3;
                            for (int offset = 1; offset <= arrowThickness; ++offset) {
                                // Calculate offset positions using the same perpendicular vector.
                                int tipX_offset = tipX + static_cast<int>(perpX * offset);
                                int tipY_offset = tipY + static_cast<int>(perpY * offset);
                                int leftWingX_offset = static_cast<int>(leftWingX) + static_cast<int>(perpX * offset);
                                int leftWingY_offset = static_cast<int>(leftWingY) + static_cast<int>(perpY * offset);
                                SDL_RenderDrawLine(renderer, tipX_offset, tipY_offset, leftWingX_offset, leftWingY_offset);

                                tipX_offset = tipX - static_cast<int>(perpX * offset);
                                tipY_offset = tipY - static_cast<int>(perpY * offset);
                                leftWingX_offset = static_cast<int>(leftWingX) - static_cast<int>(perpX * offset);
                                leftWingY_offset = static_cast<int>(leftWingY) - static_cast<int>(perpY * offset);
                                SDL_RenderDrawLine(renderer, tipX_offset, tipY_offset, leftWingX_offset, leftWingY_offset);

                                tipX_offset = tipX + static_cast<int>(perpX * offset);
                                tipY_offset = tipY + static_cast<int>(perpY * offset);
                                int rightWingX_offset = static_cast<int>(rightWingX) + static_cast<int>(perpX * offset);
                                int rightWingY_offset = static_cast<int>(rightWingY) + static_cast<int>(perpY * offset);
                                SDL_RenderDrawLine(renderer, tipX_offset, tipY_offset, rightWingX_offset, rightWingY_offset);

                                tipX_offset = tipX - static_cast<int>(perpX * offset);
                                tipY_offset = tipY - static_cast<int>(perpY * offset);
                                rightWingX_offset = static_cast<int>(rightWingX) - static_cast<int>(perpX * offset);
                                rightWingY_offset = static_cast<int>(rightWingY) - static_cast<int>(perpY * offset);
                                SDL_RenderDrawLine(renderer, tipX_offset, tipY_offset, rightWingX_offset, rightWingY_offset);
                            }
                        }
                        else {
                            // For non-selected edges, draw arrowheads.  Use the same blend mode as the edge.
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                            SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(leftWingX), static_cast<int>(leftWingY));
                            SDL_RenderDrawLine(renderer, tipX, tipY, static_cast<int>(rightWingX), static_cast<int>(rightWingY));
                        }
                    }
                }
            }
        }

        // Render vertices with transparency based on incident face visibility.
        for (const auto& vertex : mesh->vertices) {
            auto projected = project(vertex->pos, camera, viewport);
            if (projected) {
                Uint8 alpha = 255;
                bool isVisible = false;
                for (const auto& face : mesh->faces) {
                    if (std::find(face->vertices.begin(), face->vertices.end(), vertex.get()) != face->vertices.end()) {
                        if (faceVisibility[face.get()]) {
                            isVisible = true;
                            break;
                        }
                    }
                }
                if (!isVisible) {
                    alpha = 50;
                }
                // Set Blend mode for vertex based on visibility
                if (alpha == 255) {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }
                else {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                }

                // Draw the vertex as a filled rectangle.
                SDL_Rect pointRect = { projected->first - 2, projected->second - 2, 9, 9 };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha);
                SDL_RenderFillRect(renderer, &pointRect);

                if (renderText) {
                    std::string vertexLabel = "v" + std::to_string(vertex->index);
                    SDL_Color red = { 255, 0, 0, alpha };
                    SDL_Surface* textSurface = TTF_RenderText_Solid(font, vertexLabel.c_str(), red);
                    if (textSurface) {
                        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                        SDL_Rect textRect = { projected->first + 10, projected->second - 10, textSurface->w, textSurface->h };
                        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                        SDL_FreeSurface(textSurface);
                        SDL_DestroyTexture(textTexture);
                    }
                }
            }
        }

        // Render face labels at each face's centroid with alpha based on face visibility.
        for (const auto& face : mesh->faces) {
            if (face->vertices.empty())
                continue;

            vec3 centroid = vec3(0.0, 0.0, 0.0);
            for (const auto& v : face->vertices) {
                centroid += v->pos;
            }
            centroid /= static_cast<float>(face->vertices.size());

            auto projCentroid = project(centroid, camera, viewport);
            if (projCentroid && renderText) {
                std::string faceLabel = "f" + std::to_string(face->index);
                Uint8 faceAlpha = faceVisibility[face.get()] ? 255 : 50;
                // Set Blend mode for face based on visibility
                if (faceAlpha == 255) {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }
                else {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                }
                SDL_Color faceColor = { 255, 255, 0, faceAlpha };
                SDL_Surface* textSurface = TTF_RenderText_Solid(font, faceLabel.c_str(), faceColor);
                if (textSurface) {
                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    SDL_Rect textRect = { projCentroid->first, projCentroid->second, textSurface->w, textSurface->h };
                    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                    SDL_FreeSurface(textSurface);
                    SDL_DestroyTexture(textTexture);
                }
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
