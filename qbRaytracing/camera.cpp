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
#pragma omp parallel for schedule(dynamic)
    for (int l = 0; l < image_height; ++l) {
        double v = double(l) / (image_height - 1);
        vec3 vertical_component = v * vertical;
        for (int c = 0; c < image_width; ++c) {
            double u = double(c) / (image_width - 1);
            vec3 ray_direction = lower_left_corner + u * horizontal + vertical_component - origin;
            ray_direction = unit_vector(ray_direction); // Normalize the ray direction
            ray r(origin, ray_direction);
            color pixel_color = cast_ray(r, world, lights);
            write_color(pixels, c, l, image_width, image_height, pixel_color);
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

color Camera::phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world) {
    // Parâmetros de luz ambiente
    double ambient_light_intensity = 0.5;
    double k_ambient = 0.8;

    // Componente ambiente
    color ambient = k_ambient * ambient_light_intensity * rec.material->diffuse_color;

    // Inicializa cores difusa e especular
    color diffuse(0, 0, 0);
    color specular(0, 0, 0);

    // Loop através de cada luz e acumula contribuições difusas e especulares
    for (const auto& light : lights) {
        // Calcula a direção da luz e a distância até a luz
        vec3 light_dir = unit_vector(light.position - rec.p);
        double light_distance = (light.position - rec.p).length();

        ray shadow_ray(rec.p + rec.normal * 1e-3, light_dir);
        hit_record shadow_rec;

        // Se houver uma interseção com qualquer objeto antes de alcançar a luz, pula a contribuição desta luz
        if (world.hit(shadow_ray, interval(0.001, light_distance), shadow_rec)) {
            continue;
        }

        // Componente difusa
        double diff = std::max(dot(rec.normal, light_dir), 0.0);
        diffuse += rec.material->k_diffuse * diff * rec.material->diffuse_color * light.light_color * light.intensity;

        // Componente especular (não multiplica pela cor difusa)
        vec3 reflect_dir = reflect(-light_dir, rec.normal);
        double spec = std::pow(std::max(dot(view_dir, reflect_dir), 0.0), rec.material->shininess);
        specular += rec.material->k_specular * spec * light.light_color * light.intensity;
    }

    // Combina os componentes
    color final_color = ambient + diffuse + specular;
    // Limita os valores da cor entre 0 e 1
    final_color = clamp(final_color, 0.0, 1.0);

    return final_color;
}

color Camera::cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights, int depth) const {
    if (depth <= 0) {
        return color(0, 0, 0);
    }

    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        if (!rec.front_face) {
            return background_color(r);
        }

        vec3 view_dir = unit_vector(-r.direction());

        // Obtain the Phong color for the hit object
        color phong_color = phong_shading(rec, view_dir, lights, world);

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
