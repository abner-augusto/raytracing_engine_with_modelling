#pragma once

#include "raytracer.h"
#include "camera.h"
#include "render_state.h"
#include <vector>
#include <imgui.h>

// ---------------------------------------------------------------------------
// Main "menu" and global UI
// ---------------------------------------------------------------------------

void draw_menu(RenderState& render_state,
               Camera& camera, 
               HittableManager world,
               std::vector<Light>& lights
              );

void DrawFpsCounter(float fps);