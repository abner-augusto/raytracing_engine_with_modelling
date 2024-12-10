#pragma once

#include "raytracer.h"
#include "camera.h"
#include <vector>

void draw_menu(bool& render_raytracing,
    bool& show_wireframe,
    double& speed,
    Camera& camera,
    point3& origin,
    hittable_list& world);

void draw_fps_counter(float fps);
