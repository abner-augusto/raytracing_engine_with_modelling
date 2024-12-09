#pragma once

#include "raytracer.h"
#include <vector>

void draw_menu(bool& render_raytracing,
    bool& show_wireframe,
    double& speed,
    double& camera_fov,
    float camera_origin[3],
    double aspect_ratio,
    point3& origin,
    vec3& horizontal,
    vec3& vertical,
    vec3& lower_left_corner,
    double& focal_length,
    double& viewport_width,
    double& viewport_height,
    hittable_list& world);

void draw_fps_counter(float fps);
