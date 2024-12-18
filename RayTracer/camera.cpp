#include "camera.h"
#include "raytracer.h"
#include <algorithm>
#include <cmath>
#include <omp.h>

Camera::Camera(
    const point3& origin,
    const vec3& horizontal,
    const vec3& vertical,
    const point3& lower_left_corner,
    double focal_length,
    double aspect_ratio)
    : origin(origin),
    horizontal(horizontal),
    vertical(vertical),
    lower_left_corner(lower_left_corner),
    focal_length(focal_length),
    aspect_ratio(aspect_ratio) {
}

void Camera::render(Uint32* pixels, int image_width, int image_height, const hittable& world, const std::vector<Light>& lights) const {
    constexpr int TILESIZE = 16;

    // Compute the number of tiles in each dimension
    const int num_x_tiles = (image_width + TILESIZE - 1) / TILESIZE;
    const int num_y_tiles = (image_height + TILESIZE - 1) / TILESIZE;

    // Parallelize over tiles
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

                // Compute normalized coordinates
                double u = double(pixel_x) / (image_width - 1);
                double v = double(pixel_y) / (image_height - 1);

                // Compute ray direction
                vec3 ray_direction = lower_left_corner + u * horizontal + v * vertical - origin;
                ray_direction = unit_vector(ray_direction); // Normalize the ray direction

                // Cast the ray and compute pixel color
                ray r(origin, ray_direction);
                color pixel_color = cast_ray(r, world, lights);

                // Write the pixel color to the output buffer
                write_color(pixels, pixel_x, pixel_y, image_width, image_height, pixel_color);
            }
        }
    }
}


void Camera::update_basis_vectors() {
    // Recalculate viewport dimensions based on focal length
    double viewport_height = 2.0 / focal_length;
    double viewport_width = aspect_ratio * viewport_height;

    // Update camera basis vectors
    horizontal = vec3(viewport_width, 0, 0);
    vertical = vec3(0, viewport_height, 0);
    lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 - vec3(0, 0, focal_length);
}


void Camera::set_focal_length(double new_focal_length) {
    focal_length = new_focal_length;
    update_basis_vectors();
}


color Camera::background_color(const ray& r) const {
    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

static color calculate_diffuse(const vec3& normal, const vec3& light_dir, const color& diffuse_color, double k_diffuse, const color& light_color, double light_intensity) {
    double diff = std::max(dot(normal, light_dir), 0.0);
    return k_diffuse * diff * diffuse_color * light_color * light_intensity;
}

static color calculate_specular(const vec3& normal, const vec3& light_dir, const vec3& view_dir, double shininess, double k_specular, const color& light_color, double light_intensity) {
    vec3 reflect_dir = reflect(-light_dir, normal);
    double spec = std::pow(std::max(dot(view_dir, reflect_dir), 0.0), shininess);
    return k_specular * spec * light_color * light_intensity;
}

color Camera::phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world, const color& diffuse_color) {
    // Ambient light
    double ambient_light_intensity = 0.5;
    double k_ambient = 0.8;
    color ambient = k_ambient * ambient_light_intensity * diffuse_color;

    // Initialize diffuse and specular components
    color diffuse(0, 0, 0);
    color specular(0, 0, 0);

    const double shadow_bias = 1e-3;

    // Parallel loop over lights
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

        // Combine thread-local contributions
#pragma omp critical
        {
            diffuse += local_diffuse;
            specular += local_specular;
        }
    }

    // Combine components
    color final_color = ambient + diffuse + specular;
    return clamp(final_color, 0.0, 1.0);
}

color Camera::cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights, int depth) const {
    if (depth <= 0) {
        return color(0, 0, 0);
    }

    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        //if (!rec.front_face) {
        //    return background_color(r);
        //}
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