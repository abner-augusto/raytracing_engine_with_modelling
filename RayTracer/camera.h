#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>

#include "raytracer.h"
#include "light.h"

class Camera {
public:
    Camera(
        const point3& origin,
        const point3& target,
        int image_width,
        double aspect_ratio,
        double fov, // Expected in degrees
        const vec3& up = vec3(0, 1, 0)
    )
        : origin(origin), target(target), up(up),
        image_width(image_width), aspect_ratio(aspect_ratio),
        fov_vertical(degrees_to_radians(fov)),
        pixels(nullptr)
    {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // Allocate pixel buffer
        pixels = new Uint32[image_width * image_height];

        // Initialize camera basis vectors and matrices
        look_at(target, up);

        // Clear pixels initially
        clear_pixels();
    }

    ~Camera() {
        delete[] pixels;
    }

    void look_at(const point3& target, const vec3& up = vec3(0, 1, 0)) {
        this->target = target;
        this->up = up;

        // Compute the new basis vectors
        w = unit_vector(origin - target); // Camera backward
        u = unit_vector(cross(up, w));    // Camera right
        v = cross(w, u);                  // Camera up

        // Handle collinear `up` and `w` case
        if (cross(up, w).length() < 1e-6) {
            this->up = vec3(1, 0, 0); // Choose an alternative up vector
            u = unit_vector(cross(this->up, w));
            v = cross(w, u);
        }

        // Update focal length and viewport
        focal_length = 0.5 * image_height / tan(fov_vertical / 2.0);
        viewport_height = 2.0 * tan(fov_vertical / 2.0) * focal_length;
        viewport_width = aspect_ratio * viewport_height;

        // Recalculate lower_left_corner
        lower_left_corner = origin - u * viewport_width / 2.0 - v * viewport_height / 2.0 - w * focal_length;

        // Update the world-to-camera matrix
        update_world_to_camera_matrix();
    }

    void update_world_to_camera_matrix() {
        // Camera's rotation matrix
        Matrix4x4 camera_rotation;
        camera_rotation.m[0][0] = u.x();
        camera_rotation.m[1][0] = u.y();
        camera_rotation.m[2][0] = u.z();

        camera_rotation.m[0][1] = v.x();
        camera_rotation.m[1][1] = v.y();
        camera_rotation.m[2][1] = v.z();

        camera_rotation.m[0][2] = w.x();
        camera_rotation.m[1][2] = w.y();
        camera_rotation.m[2][2] = w.z();

        // Camera's translation matrix
        Matrix4x4 camera_translation = Matrix4x4::translation(-origin.x(), -origin.y(), -origin.z());

        // World-to-camera matrix is the combination of rotation and translation
        world_to_camera_matrix = camera_rotation * camera_translation;

        // Debug world-to-camera matrix
        //world_to_camera_matrix.print();
    }



    Matrix4x4 get_world_to_camera_matrix() const {
        return world_to_camera_matrix;
    }

    void clear_pixels() {
        std::fill(pixels, pixels + (image_width * image_height), 0);
    }

    void render(
        const hittable& world,
        const std::vector<Light>& lights,
        int samples_per_pixel = 1,
        bool enable_antialias = false
    ) const {
        int TILESIZE = std::min(32, image_width / 10); // Adjust tile size based on image width

        // Compute the number of tiles in each dimension
        const int num_x_tiles = (image_width + TILESIZE - 1) / TILESIZE;
        const int num_y_tiles = (image_height + TILESIZE - 1) / TILESIZE;

#pragma omp parallel for schedule(dynamic)
        for (int tile_index = 0; tile_index < num_x_tiles * num_y_tiles; ++tile_index) {
            // Compute the starting pixel coordinates of the current tile
            const int tile_x = (tile_index % num_x_tiles) * TILESIZE;
            const int tile_y = (tile_index / num_x_tiles) * TILESIZE;

            // Iterate over each pixel in the tile
            for (int j = 0; j < TILESIZE; ++j) {
                for (int i = 0; i < TILESIZE; ++i) {
                    // Compute the actual pixel coordinates
                    int pixel_x = tile_x + i;
                    int pixel_y = tile_y + j;

                    // Ensure the pixel is within the image bounds
                    if (pixel_x >= image_width || pixel_y >= image_height) continue;

                    color accumulated_color(0, 0, 0);

                    // Determine the number of samples to use (1 if AA is disabled)
                    int spp = enable_antialias ? samples_per_pixel : 1;

                    for (int s = 0; s < spp; s++) {
                        // Compute random offsets for anti-aliasing if enabled, else center pixel sample
                        double u_pixel = (static_cast<double>(pixel_x) + (enable_antialias ? random_double(0.0, 1.0) : 0.5)) / (image_width - 1);
                        double v_pixel = (static_cast<double>(pixel_y) + (enable_antialias ? random_double(0.0, 1.0) : 0.5)) / (image_height - 1);

                        // Calculate ray direction through the pixel
                        vec3 pixel_on_sensor = lower_left_corner + u * (viewport_width * u_pixel) + v * (viewport_height * v_pixel);
                        vec3 ray_direction = unit_vector(pixel_on_sensor - origin);

                        ray r(origin, ray_direction);
                        accumulated_color += cast_ray(r, world, lights);
                    }

                    // Average the color if AA is enabled
                    accumulated_color *= (1.0 / spp);

                    // Write the pixel color to the output buffer
                    write_color(pixels, pixel_x, pixel_y, image_width, image_height, accumulated_color);
                }
            }
        }
    }

    // Setter for origin
    void set_origin(const point3& new_origin) {
        origin = new_origin;
        look_at(target, up);
    }

    // Setter for focal length
    void set_fov(double fov_in_degrees) {
        fov_vertical = degrees_to_radians(fov_in_degrees);
        look_at(target, up); // Recalculate camera basis vectors and matrices
    }

    // Setter for image width
    void set_image_width(int new_image_width) {
        image_width = new_image_width;
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // Reallocate pixel buffer
        delete[] pixels;
        pixels = new Uint32[image_width * image_height];

        clear_pixels();
        look_at(target, up);
    }

    // Setter for the look-at point
    void set_look_at(const point3& new_target, const vec3& new_up = vec3(0, 1, 0)) {
        target = new_target;
        up = new_up;

        look_at(target, up);
    }
    
    // Accessors of camera
    int get_image_width() const { return image_width; }
    int get_image_height() const { return image_height; }
    point3 get_origin() const { return origin; }
    double get_focal_length() const { return 0.5 * image_height / tan(fov_vertical / 2.0); }
    vec3 get_horizontal() const { return u; } // Renamed to u
    vec3 get_vertical() const { return v; }   // Renamed to v
    vec3 get_look_at() const { return target; }
    vec3 get_u() const { return u; }
    vec3 get_v() const { return v; }
    vec3 get_w() const { return w; }
    Uint32* get_pixels() const { return pixels; }
    double get_viewport_width() const { return viewport_width; }
    double get_viewport_height() const { return viewport_height; }
    double get_fov_degrees() const { return radians_to_degrees(fov_vertical); }

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

    color phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world, const color& diffuse_color) const {
        // Ambient light
        double ambient_light_intensity = 0.4;
        //color ambient_light_color(0.8, 0.85, 1.0);  // Soft blue skylight
        color ambient_light_color(1.0, 0.95, 0.8);  // Warm yellow

        color ambient = ambient_light_intensity * ambient_light_color * diffuse_color;

        // Initialize diffuse and specular components
        color diffuse(0, 0, 0);
        color specular(0, 0, 0);
        const double shadow_bias = 1e-3;

#pragma omp parallel
        {
            color local_diffuse(0, 0, 0);
            color local_specular(0, 0, 0);
#pragma omp for
            for (int i = 0; i < static_cast<int>(lights.size()); ++i) {
                const auto& light = lights[i];
                vec3 light_dir = unit_vector(light.position - rec.p);
                double light_distance = (light.position - rec.p).length();

                // Skip lights that don't contribute (back-facing)
                if (dot(rec.normal, light_dir) <= 0) {
                    continue;
                }

                // Shadow check
                ray shadow_ray(rec.p + rec.normal * shadow_bias, light_dir);
                hit_record shadow_rec;
                if (world.hit(shadow_ray, interval(0.001, light_distance), shadow_rec)) {
                    continue;
                }

                // Diffuse and specular contributions
                double attenuation = 1.0 / (1.0 + 0.1 * light_distance + 0.01 * light_distance * light_distance);
                local_diffuse += calculate_diffuse(rec.normal, light_dir, diffuse_color, rec.material->k_diffuse, light.light_color, light.intensity) * attenuation;
                local_specular += calculate_specular(rec.normal, light_dir, view_dir, rec.material->shininess, rec.material->k_specular, light.light_color, light.intensity) * attenuation;
            }
#pragma omp critical
            {
                diffuse += local_diffuse;
                specular += local_specular;
            }
        }

        // Combine components
        color final_color = ambient + diffuse + specular;
        return final_color;
    }

    color cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights, int depth = 50) const {
        if (depth <= 0) {
            return color(0, 0, 0);
        }

        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec)) {
            vec3 view_dir = unit_vector(-r.direction());

            // Use the material's color, either from the texture or as a solid color
            color diffuse_color = rec.material->get_color(rec.u, rec.v);

            // Obtain the Phong color for the hit object, now using the texture or solid color
            color phong_color = phong_shading(rec, view_dir, lights, world, diffuse_color);

            // Calculate reflection if the material supports it
            if (rec.material->reflection > 0.0) {
                vec3 reflected_dir = reflect(unit_vector(r.direction()), rec.normal);
                ray reflected_ray(rec.p + rec.normal * 1e-3, reflected_dir);

                color reflected_color = cast_ray(reflected_ray, world, lights, depth - 1);

                // Combine Phong color with reflection
                return (1.0 - rec.material->reflection) * phong_color +
                    rec.material->reflection * reflected_color;
            }

            return phong_color;
        }

        return background_color(r);
    }

    // Camera attributes
    point3 origin;
    point3 target;
    vec3 up;

    int image_width;
    int image_height;
    double aspect_ratio;
    double fov_vertical;
    double focal_length;
    double viewport_width;
    double viewport_height;
    Uint32* pixels;
    Matrix4x4 world_to_camera_matrix;

    // Cached basis vectors
    vec3 u, v, w;
    point3 lower_left_corner;
};

#endif // CAMERA_H