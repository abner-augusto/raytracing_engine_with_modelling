#ifndef LIGHT_H
#define LIGHT_H

#include "color.h"
#include "vec3.h"
#include "matrix4x4.h"
#include <cmath>

class Light {
public:
    Light(const vec3& pos, double inten, const color& col)
        : position(pos), intensity(inten), light_color(col) {
    }

    virtual ~Light() = default;

    virtual vec3 get_light_direction(const vec3& point) const = 0;
    virtual double get_attenuation(const vec3& point) const = 0;
    virtual std::string get_type_name() const = 0;

    virtual void transform(const Matrix4x4& matrix) {
        position = matrix.transform_point(position);
    }

    // Getters and setters...
    vec3 get_position() const { return position; }
    void set_position(const vec3& pos) { position = pos; }
    double get_intensity() const { return intensity; }
    void set_intensity(double inten) { intensity = inten; }
    color get_color() const { return light_color; }
    void set_color(const color& col) { light_color = col; }

private:
    vec3 position;
    double intensity;
    color light_color;
};

class PointLight : public Light {
public:
    PointLight(const vec3& pos, double inten, const color& col)
        : Light(pos, inten, col) {
    }

    vec3 get_light_direction(const vec3& point) const override {
        return (get_position() - point).normalized();
    }

    double get_attenuation(const vec3& point) const override {
        double distance = (get_position() - point).length();
        return 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
    }

    std::string get_type_name() const override {
        return "Point Light";
    }
};

class DirectionalLight : public Light {
public:
    DirectionalLight(const vec3& dir, double inten, const color& col)
        : Light(vec3(0, 0, 0), inten, col), direction(dir.normalized()) {
    }

    vec3 get_light_direction(const vec3& point) const override {
        return -direction;
    }

    double get_attenuation(const vec3& point) const override {
        return 1.0;
    }

    std::string get_type_name() const override {
        return "Directional Light";
    }

    vec3 get_direction() const { return direction; }
    void set_direction(const vec3& dir) { direction = dir.normalized(); }

    void transform(const Matrix4x4& matrix) override {
        direction = matrix.transform_vector(direction).normalized();
    }

private:
    vec3 direction;
};

class SpotLight : public Light {
public:
    SpotLight(const vec3& pos, const vec3& dir, double inten, const color& col,
        double cutoff_degrees = 45.0, double outer_cutoff_degrees = 50.0)
        : Light(pos, inten, col), direction(dir.normalized()) {
        set_cutoff_angles(cutoff_degrees, outer_cutoff_degrees);
    }

    vec3 get_light_direction(const vec3& point) const override {
        return (get_position() - point).normalized();
    }

    double get_attenuation(const vec3& point) const override {
        vec3 light_dir = -get_light_direction(point);
        double cos_angle = dot(light_dir, direction);
        if (cos_angle < cos_outer_cutoff) return 0.0;

        double distance = (get_position() - point).length();
        double attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);

        if (cos_angle > cos_cutoff_angle) return attenuation;

        double t = (cos_angle - cos_outer_cutoff) / (cos_cutoff_angle - cos_outer_cutoff);
        double intensity_factor = t * t * (3.0 - 2.0 * t);
        return attenuation * intensity_factor;
    }

    std::string get_type_name() const override {
        return "Spot Light";
    }

    vec3 get_direction() const { return direction; }
    void set_direction(const vec3& dir) { direction = dir.normalized(); }

    double get_inner_cutoff() const { return acos(cos_cutoff_angle) * 180.0 / M_PI; }
    double get_outer_cutoff() const { return acos(cos_outer_cutoff) * 180.0 / M_PI; }
    void set_cutoff_angles(double inner, double outer) {
        cos_cutoff_angle = std::cos(inner * M_PI / 180.0);
        cos_outer_cutoff = std::cos(outer * M_PI / 180.0);
    }

    void transform(const Matrix4x4& matrix) override {
        Light::transform(matrix); 
        direction = matrix.transform_vector(direction).normalized();
    }

private:
    vec3 direction;
    double cos_cutoff_angle;
    double cos_outer_cutoff;
};

#endif