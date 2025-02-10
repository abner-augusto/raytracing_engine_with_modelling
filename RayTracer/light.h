#ifndef LIGHT_H
#define LIGHT_H

#include "color.h"
#include "vec3.h"
#include "matrix4x4.h"
#include <cmath>

// Abstract base class for all light types
class Light {
public:
    vec3 position;
    double intensity;
    color light_color;

    Light(const vec3& pos, double inten, const color& col)
        : position(pos), intensity(inten), light_color(col) {
    }

    virtual ~Light() = default;

    // Pure virtual function to get light direction at a point
    virtual vec3 get_light_direction(const vec3& point) const = 0;

    // Pure virtual function to get light attenuation at a point
    virtual double get_attenuation(const vec3& point) const = 0;

    void transform(const Matrix4x4& matrix) {
        position = matrix.transform_point(position);
    }
};

// Point light implementation
class PointLight : public Light {
public:
    PointLight(const vec3& pos, double inten, const color& col)
        : Light(pos, inten, col) {
    }

    vec3 get_light_direction(const vec3& point) const override {
        return (position - point).normalized();
    }

    double get_attenuation(const vec3& point) const override {
        double distance = (position - point).length();
        //  Quadratic attenuation.
        return 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
    }
};

// Directional light (like the sun)
class DirectionalLight : public Light {
private:
    vec3 direction;

public:
    DirectionalLight(const vec3& dir, double inten, const color& col)
        : Light(vec3(0, 0, 0), inten, col), direction(dir.normalized()) {
    } // Normalize on construction

    vec3 get_light_direction(const vec3& point) const override {
        return -direction;  // Light rays are parallel
    }

    double get_attenuation(const vec3& point) const override {
        return 1.0;  // No attenuation
    }

    void transform(const Matrix4x4& matrix) {
        // Transform direction (it's a vector, not a point)
        direction = matrix.transform_vector(direction).normalized();
    }
};

// Spot light (like a flashlight)
class SpotLight : public Light {
private:
    vec3 direction;
    double cos_cutoff_angle;      // Store cosine of the angle
    double cos_outer_cutoff;      // Store cosine of the angle

public:
    SpotLight(const vec3& pos, const vec3& dir, double inten, const color& col,
        double cutoff_degrees = 45.0, double outer_cutoff_degrees = 50.0)
        : Light(pos, inten, col),
        direction(dir.normalized()) {
        cos_cutoff_angle = std::cos(cutoff_degrees * M_PI / 180.0);
        cos_outer_cutoff = std::cos(outer_cutoff_degrees * M_PI / 180.0);
    }

    vec3 get_light_direction(const vec3& point) const override {
        return (position - point).normalized();
    }

    double get_attenuation(const vec3& point) const override {
        vec3 light_dir = -get_light_direction(point);
        double cos_angle = dot(light_dir, direction);

        // Outside the outer cone
        if (cos_angle < cos_outer_cutoff) return 0.0;

        // Distance attenuation
        double distance = (position - point).length();
        double attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);

        // Inside the inner cone.
        if (cos_angle > cos_cutoff_angle) return attenuation;

        // Smoothstep for a smoother falloff
        double t = (cos_angle - cos_outer_cutoff) / (cos_cutoff_angle - cos_outer_cutoff);
        //  Apply a smoothstep function (cubic Hermite interpolation).
        double intensity_factor = t * t * (3.0 - 2.0 * t);
        return attenuation * intensity_factor;
    }
    void transform(const Matrix4x4& matrix) {
        Light::transform(matrix); // Transform position
        direction = matrix.transform_vector(direction).normalized(); // Transform and normalize direction
    }

};

#endif