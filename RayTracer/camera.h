#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"
#include "vec3.h"
#include "hittable.h"
#include "light.h"
#include <vector>

class Camera {
public:
    // Constructor
    Camera(
        const point3& origin,
        const vec3& horizontal,
        const vec3& vertical,
        const point3& lower_left_corner,
        double focal_length,
        double aspect_ratio);

    // Public methods
    void render(Uint32* pixels, int image_width, int image_height, const hittable& world, const std::vector<Light>& lights) const;

    void set_focal_length(double new_focal_length);

    void update_basis_vectors(); // New private function

    // Public variables
    point3 origin;             // Camera position
    vec3 horizontal;           // Horizontal viewport dimension
    vec3 vertical;             // Vertical viewport dimension
    point3 lower_left_corner;  // Lower left corner of the viewport
    double focal_length;       // Camera focal length
    double aspect_ratio;       // Aspect ratio of the camera

private:
    // Private methods
    color cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights, int depth = 4) const;
    color background_color(const ray& r) const;
    static color phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world, const color& diffuse_color);
};

#endif // CAMERA_H
