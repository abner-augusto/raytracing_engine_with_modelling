#pragma once

#include <vector>
#include <imgui.h>
#include <cmath>

#include "camera.h"
#include "render_state.h"
#include "csg.h"
#include "hittable_manager.h"
#include "sphere.h"
#include "box_csg.h"
#include "cylinder.h"
#include "cone.h"

void draw_menu(RenderState& render_state,
               Camera& camera, 
               HittableManager world,
               std::vector<Light>& lights
              );

void DrawFpsCounter(float fps);

void ShowHittableManagerUI(HittableManager& world);

void ShowInfoWindow(HittableManager& world);