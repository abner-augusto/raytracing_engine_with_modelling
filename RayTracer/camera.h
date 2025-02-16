#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include <iostream>
#include <string>

#include "raytracer.h"
#include "light.h"

class Camera {
public:
    Matrix4x4 world_to_camera_matrix;
    Matrix4x4 camera_to_world_matrix;
    using ProjectionFunction = ray(Camera::*)(int, int, double, double) const;

    Camera(const point3& origin, const point3& at, int image_width, double aspect_ratio, double fov)
        : origin(origin), look_at(at), world_up(0, 1, 0), image_width(image_width), aspect_ratio(aspect_ratio), fov(fov), current_projection(&Camera::compute_ray_at)
    {
        // Compute image height
        image_height = static_cast<int>(image_width / aspect_ratio);

        // Allocate pixel buffer
        pixels = new Uint32[image_width * image_height];

        // Compute the transformation matrix
        calculate_axes();
        calculate_matrices();

        // Clear pixels initially
        clear_pixels();
    }

    ~Camera() {
        delete[] pixels;
    }

    void clear_pixels() {
        std::fill(pixels, pixels + (image_width * image_height), 0);
    }

    void calculate_axes() {
        forward = unit_vector(origin - look_at);         // Negative look direction
        right = unit_vector(cross(world_up, forward));   // Perpendicular to up and forward
        up = cross(forward, right);                      // True up vector (recalculated)
    }

    void calculate_matrices() {
        // Camera-to-World Matrix
        camera_to_world_matrix = Matrix4x4(
            right.x(), up.x(), forward.x(), origin.x(),
            right.y(), up.y(), forward.y(), origin.y(),
            right.z(), up.z(), forward.z(), origin.z(),
            0.0,       0.0,    0.0,         1.0
        );

        // World-to-Camera Matrix
        world_to_camera_matrix = Matrix4x4(
            right.x(),   right.y(),   right.z(),   -dot(right, origin),
            up.x(),      up.y(),      up.z(),      -dot(up, origin),
            forward.x(), forward.y(), forward.z(), -dot(forward, origin),
            0.0,         0.0,         0.0,          1.0
        );
    }

    void tilt(double angle, const std::string& plane = "ZY") {
        // Convert angle to radians
        double rad = angle * M_PI / 180.0;

        Matrix4x4 tilt_matrix;

        if (plane == "ZY") {
            // Rotate on the ZY plane (around the right vector)
            tilt_matrix = Matrix4x4(
                1.0, 0.0,      0.0,       0.0,
                0.0, cos(rad), -sin(rad), 0.0,
                0.0, sin(rad), cos(rad),  0.0,
                0.0, 0.0,      0.0,       1.0
            );

            // Apply tilt to forward and up
            forward = (tilt_matrix * vec4(forward, 0.0)).to_vec3();
            up = (tilt_matrix * vec4(up, 0.0)).to_vec3();
        }
        else if (plane == "XY") {
            // Rotate on the XY plane (around the forward vector)
            tilt_matrix = Matrix4x4(
                cos(rad), -sin(rad), 0.0, 0.0,
                sin(rad), cos(rad),  0.0, 0.0,
                0.0,      0.0,       1.0, 0.0,
                0.0,      0.0,       0.0, 1.0
            );

            // Apply tilt to up and right
            up = (tilt_matrix * vec4(up, 0.0)).to_vec3();
            right = (tilt_matrix * vec4(right, 0.0)).to_vec3();
        }
        else {
            throw std::invalid_argument("Unsupported plane. Use 'ZY' or 'XY'.");
        }

        // Recalculate the matrices to reflect the new orientation
        calculate_matrices();
    }

    void render(
        HittableManager& manager,
        int samples_per_pixel = 1,
        bool enable_antialias = false
    ) const {
        int TILESIZE = std::min(32, image_width / 10);

        // Compute the number of tiles in each dimension
        const int num_x_tiles = (image_width + TILESIZE - 1) / TILESIZE;
        const int num_y_tiles = (image_height + TILESIZE - 1) / TILESIZE;

        // Precompute perspective parameters
        double fov_radians = degrees_to_radians(fov);
        double half_fov = 0.5 * fov_radians;
        double tan_half_fov = std::tan(half_fov);
        double screen_z = -1.0;

#pragma omp parallel for schedule(dynamic)
        for (int tile_index = 0; tile_index < num_x_tiles * num_y_tiles; ++tile_index) {
            const int tile_x = (tile_index % num_x_tiles) * TILESIZE;
            const int tile_y = (tile_index / num_x_tiles) * TILESIZE;

            for (int j = 0; j < TILESIZE; ++j) {
                for (int i = 0; i < TILESIZE; ++i) {
                    int pixel_x = tile_x + i;
                    int pixel_y = tile_y + j;

                    if (pixel_x >= image_width || pixel_y >= image_height)
                        continue;

                    color accumulated_color(0, 0, 0);
                    int spp = enable_antialias ? samples_per_pixel : 1;

                    for (int s = 0; s < spp; s++) {
                        double offset_x = enable_antialias ? random_double(0.0, 1.0) : 0.5;
                        double offset_y = enable_antialias ? random_double(0.0, 1.0) : 0.5;

                        // Use the projection_function_ptr to compute the ray
                        ray r = (this->*current_projection)(pixel_x, pixel_y, offset_x, offset_y);

                        // Cast ray and accumulate color.  Get lights from manager.
                        accumulated_color += shade_ray_at_hit(r, manager, 5, renderShadows);
                    }

                    // Average color and write to pixel buffer
                    accumulated_color *= (1.0 / spp);
                    int flipped_pixel_y = image_height - 1 - pixel_y;
                    write_color(pixels, pixel_x, flipped_pixel_y, image_width, image_height, accumulated_color);
                }
            }
        }
    }


    // compute perspective ray
    ray compute_ray_at(int pixel_x, int pixel_y, double offset_x = 0.5, double offset_y = 0.5) const {
        // Precompute perspective parameters
        double fov_radians = degrees_to_radians(fov);
        double half_fov = 0.5 * fov_radians;
        double tan_half_fov = std::tan(half_fov);

        // Convert the pixel coordinate to normalized device coordinates (NDC)
        double ndc_x = (static_cast<double>(pixel_x) + offset_x) / image_width;
        double ndc_y = (static_cast<double>(pixel_y) + offset_y) / image_height;

        // Map NDC to screen space
        double screen_x = (2.0 * ndc_x - 1.0) * aspect_ratio * tan_half_fov;
        double screen_y = (1.0 - 2.0 * ndc_y) * tan_half_fov;
        double screen_z = -1.0;

        vec3 ray_origin, ray_direction;

        if (isCameraSpace) {
            ray_origin = vec3(0.0, 0.0, 0.0);
            ray_direction = unit_vector(vec3(screen_x, screen_y, screen_z));
        }
        else {
            vec4 dir_cam(screen_x, screen_y, screen_z, 0.0);
            vec4 dir_world = camera_to_world_matrix * dir_cam;
            ray_direction = unit_vector(dir_world.to_vec3());

            vec4 origin_cam(0.0, 0.0, 0.0, 1.0);
            vec4 origin_world = camera_to_world_matrix * origin_cam;
            ray_origin = origin_world.to_vec3();
        }

        return ray(ray_origin, ray_direction);
    }

    ray compute_orthographic_ray(int pixel_x, int pixel_y, double offset_x, double offset_y) const {
        double ndc_x = (static_cast<double>(pixel_x) + offset_x) / image_width;
        double ndc_y = (static_cast<double>(pixel_y) + offset_y) / image_height;

        double screen_x = (2.0 * ndc_x - 1.0) * aspect_ratio;
        double screen_y = (1.0 - 2.0 * ndc_y);

        vec3 ray_origin = origin + (screen_x * right) + (screen_y * up);
        vec3 ray_direction = -forward;

        return ray(ray_origin, ray_direction);
    }

    void rotate_to_isometric_view() {
        calculate_axes();
        // Isometric angles in radians
        double angle_y = degrees_to_radians(45.0);
        double angle_x = degrees_to_radians(35.264);

        // Create quaternions for Y and X rotations
        vec4 qY = vec4().createQuaternion(vec3(0, 1, 0), angle_y);
        vec4 qX = vec4().createQuaternion(vec3(1, 0, 0), angle_x);

        // Combine rotations (Y first, then X)
        vec4 qCombined = qX * qY;

        // Convert to rotation matrix and apply
        Matrix4x4 rotationMatrix = Matrix4x4::quaternion(qCombined);

        // Apply rotation to camera vectors
        forward = rotationMatrix.transform_vector(forward);
        right = rotationMatrix.transform_vector(right);
        up = rotationMatrix.transform_vector(up);

        calculate_matrices();
    }

    // Setters
    void set_origin(const point3& new_origin) {
        origin = new_origin;
        calculate_axes();
        calculate_matrices();
    }

    void set_look_at(const point3& new_look_at) {
        look_at = new_look_at;
        calculate_axes();
        calculate_matrices();
    }

    void set_fov(double new_fov) {
        if (new_fov < 10.0 || new_fov > 120.0) {
            throw std::invalid_argument("FOV must be between 10 and 120 degrees.");
        }
        fov = new_fov;
        calculate_axes();
        calculate_matrices();
    }

    void set_image_width(int new_image_width) {
        if (new_image_width <= 100) {
            throw std::invalid_argument("Image width must be greater than 100.");
        }

        image_width = new_image_width;
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // Reallocate pixel buffer
        delete[] pixels;
        pixels = new Uint32[image_width * image_height];

        clear_pixels();
        calculate_axes();
        calculate_matrices();
    }

    void transform(const Matrix4x4& matrix) {
        origin = matrix.transform_point(origin);
        look_at = matrix.transform_point(look_at);
        calculate_axes();
        calculate_matrices();
    }

    // Toggle shadows on or off
    void toggleShadows() {
        renderShadows = !renderShadows;
    }

    // Toggle Camera Space on or off
    void toggleCameraSpace() {
        isCameraSpace = !isCameraSpace;
    }

    void use_orthographic_projection() {
        current_projection = &Camera::compute_orthographic_ray;
        std::cout << "Using Orthographic Projection" << std::endl;
    }

    void use_perspective_projection() {
        calculate_axes();
        calculate_matrices();
        current_projection = &Camera::compute_ray_at;
        std::cout << "Using Perspective Projection" << std::endl;
    }

    // Accessors
    point3 get_origin() const { return origin; }
    point3 get_look_at() const { return look_at; }
    double get_fov_degrees() const { return fov; }
    int get_image_width() const { return image_width; }
    int get_image_height() const { return image_height; }
    Uint32* get_pixels() const { return pixels; }
    vec3 get_right() const { return right; }
    vec3 get_up() const { return up; }
    vec3 get_forward() const { return forward; }
    bool shadowStatus() const { return renderShadows; }
    bool CameraSpaceStatus() const { return isCameraSpace; }

private:

    color background_color(const ray& r) const {
        vec3 unit_direction = unit_vector(r.direction());
        auto t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
    }

    static inline color calculate_diffuse(const vec3& normal, const vec3& light_dir, const color& diffuse_color, double k_diffuse, const color& light_color, double light_intensity) {
        double diff = std::max(dot(normal, light_dir), 0.0);
        return k_diffuse * diff * diffuse_color * light_color * light_intensity;
    }

    static inline color calculate_specular(const vec3& normal, const vec3& light_dir, const vec3& view_dir, double shininess, double k_specular, const color& light_color, double light_intensity) {
        vec3 reflect_dir = reflect(-light_dir, normal);
        double spec = std::pow(std::max(dot(view_dir, reflect_dir), 0.0), shininess);
        return k_specular * spec * light_color * light_intensity;
    }

    color phong_shading(const hit_record& rec, const vec3& view_dir,
        const hittable& world, const color& diffuse_color, bool renderShadows) const
    {
        // Ambient light
        double ambient_light_intensity = 0.4;
        color ambient_light_color(1.0, 0.95, 0.8);  // Warm yellow
        color ambient = ambient_light_intensity * ambient_light_color * diffuse_color;

        // Initialize diffuse and specular components
        color diffuse(0, 0, 0);
        color specular(0, 0, 0);
        const double shadow_bias = 1e-3;

        // Get lights directly from the world (HittableManager)
        const HittableManager* manager_ptr = dynamic_cast<const HittableManager*>(&world);
        if (!manager_ptr) {
            return color(0, 0, 0);
        }
        const auto& lights = manager_ptr->get_lights();

        // Only parallelize if we have enough lights to justify the overhead
        if (lights.size() > 4) {
            int available_threads = omp_get_num_threads();
            int shading_threads = std::max(1, available_threads / 2);

#pragma omp parallel num_threads(shading_threads) if(lights.size() > 4)
            {
                color local_diffuse(0, 0, 0);
                color local_specular(0, 0, 0);

#pragma omp for nowait
                for (int i = 0; i < static_cast<int>(lights.size()); ++i) {
                    const auto& light = lights[i];
                    vec3 light_dir = light->get_light_direction(rec.p);

                    // Skip lights that don't contribute (back-facing)
                    if (dot(rec.normal, light_dir) <= 0) {
                        continue;
                    }

                    // Shadow check
                    if (renderShadows) {
                        ray shadow_ray(rec.p + rec.normal * shadow_bias, light_dir);
                        hit_record shadow_rec;

                        double max_distance = (dynamic_cast<DirectionalLight*>(light.get()) != nullptr) ?
                            infinity : (light->get_position() - rec.p).length();

                        if (world.hit(shadow_ray, interval(0.001, max_distance), shadow_rec)) {
                            continue;
                        }
                    }
                    // Get attenuation from the light
                    double attenuation = light->get_attenuation(rec.p);

                    // Diffuse contribution using getters
                    local_diffuse += calculate_diffuse(
                        rec.normal,
                        light_dir,
                        diffuse_color,
                        rec.material->k_diffuse,
                        light->get_color(),
                        light->get_intensity()
                    ) * attenuation;

                    // Specular contribution using getters
                    local_specular += calculate_specular(
                        rec.normal,
                        light_dir,
                        view_dir,
                        rec.material->shininess,
                        rec.material->k_specular,
                        light->get_color(),
                        light->get_intensity()
                    ) * attenuation;
                }

#pragma omp critical
                {
                    diffuse += local_diffuse;
                    specular += local_specular;
                }
            }
        }
        else {
            // Sequential processing for a small number of lights
            for (const auto& light : lights) {
                vec3 light_dir = light->get_light_direction(rec.p);

                // Skip lights that don't contribute (back-facing)
                if (dot(rec.normal, light_dir) <= 0) {
                    continue;
                }

                // Shadow check
                if (renderShadows) {
                    ray shadow_ray(rec.p + rec.normal * shadow_bias, light_dir);
                    hit_record shadow_rec;

                    double max_distance = (dynamic_cast<DirectionalLight*>(light.get()) != nullptr) ?
                        infinity : (light->get_position() - rec.p).length();

                    if (world.hit(shadow_ray, interval(0.001, max_distance), shadow_rec)) {
                        continue;
                    }
                }

                double attenuation = light->get_attenuation(rec.p);

                // Diffuse contribution using getters
                diffuse += calculate_diffuse(
                    rec.normal,
                    light_dir,
                    diffuse_color,
                    rec.material->k_diffuse,
                    light->get_color(),
                    light->get_intensity()
                ) * attenuation;

                // Specular contribution using getters
                specular += calculate_specular(
                    rec.normal,
                    light_dir,
                    view_dir,
                    rec.material->shininess,
                    rec.material->k_specular,
                    light->get_color(),
                    light->get_intensity()
                ) * attenuation;
            }
        }

        // Combine components
        return ambient + diffuse + specular;
    }

    color shade_ray_at_hit(const ray& r, const hittable& world, int depth = 5, bool renderShadows = true) const {
        if (depth <= 0) {
            return color(0, 0, 0);
        }

        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec)) {
            vec3 view_dir = unit_vector(-r.direction());

            // Use the material's color, either from the texture or as a solid color
            color diffuse_color = rec.material->get_color(rec.u, rec.v);

            // Obtain the Phong color for the hit object, now using the texture or solid color. Remove lights from here.
            color phong_color = phong_shading(rec, view_dir, world, diffuse_color, renderShadows);

            // Calculate reflection if the material supports it
            if (rec.material->reflection > 0.0) {
                vec3 reflected_dir = reflect(unit_vector(r.direction()), rec.normal);
                ray reflected_ray(rec.p + rec.normal * 1e-3, reflected_dir);

                color reflected_color = shade_ray_at_hit(reflected_ray, world, depth - 1, renderShadows);

                // Combine Phong color with reflection
                return (1.0 - rec.material->reflection) * phong_color +
                    rec.material->reflection * reflected_color;
            }

            return phong_color;
        }

        return background_color(r);
    }

    // Camera attributes
    point3 origin;          // Camera position (Eye)
    point3 look_at;         // Look-at point (At)
    vec3 world_up;          // World Up vector (Up)
    double fov;             // Field of view
    int image_width;
    int image_height;
    double aspect_ratio;
    bool isCameraSpace = false;
    bool renderShadows = true;

    ProjectionFunction current_projection;

    // Camera Axis
    vec3 right;
    vec3 up;
    vec3 forward;

    Uint32* pixels;
};

#endif // CAMERA_H