#ifndef VEC4_H
#define VEC4_H

#include <cmath>
#include <iostream>

#include "vec3.h"

class vec4 {
public:
    double x, y, z, w;

    // Constructors
    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(double x, double y, double z, double w = 1.0) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3& v, double w = 1.0) : x(v.x()), y(v.y()), z(v.z()), w(w) {}

    // Addition
    vec4 operator+(const vec4& other) const {
        return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    // Subtraction
    vec4 operator-(const vec4& other) const {
        return vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    // Scalar Multiplication
    vec4 operator*(double scalar) const {
        return vec4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    vec4& operator*=(double scalar) {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }

    // Scalar Division
    vec4 operator/(double scalar) const {
        return vec4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    vec4& operator/=(double scalar) {
        x /= scalar; y /= scalar; z /= scalar; w /= scalar;
        return *this;
    }

    // Equality
    bool operator==(const vec4& other) const {
        const double epsilon = 1e-8;
        return std::fabs(x - other.x) < epsilon &&
            std::fabs(y - other.y) < epsilon &&
            std::fabs(z - other.z) < epsilon &&
            std::fabs(w - other.w) < epsilon;
    }

    // Inequality
    bool operator!=(const vec4& other) const {
        return !(*this == other);
    }

    // Min Element-Wise
    vec4 min_(const vec4& other) const {
        return vec4(std::min(x, other.x), std::min(y, other.y), std::min(z, other.z), std::min(w, other.w));
    }

    // Max Element-Wise
    vec4 max_(const vec4& other) const {
        return vec4(std::max(x, other.x), std::max(y, other.y), std::max(z, other.z), std::max(w, other.w));
    }

    // Convert to Cartesian Coordinates (normalize by w)
    vec4 to_cartesian() const {
        if (w != 0) return vec4(x / w, y / w, z / w, 1.0);
        return *this; // If w == 0, return as is (vector)
    }

    // Convert to vec3 (Cartesian coordinates)
    vec3 to_vec3() const {
        if (w != 0) {
            return vec3(x / w, y / w, z / w);
        }
        return vec3(x, y, z);  // Direction vector
    }

    // Friend for Stream Insertion
    friend std::ostream& operator<<(std::ostream& os, const vec4& v) {
        return os << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    }

    vec4 createQuaternion(const vec3& u, double angle) {
        // Convert angle to radians and calculate half-angle
        double halfAngle = angle * M_PI / 360.0;
        double sinHalfAngle = std::sin(halfAngle);
        double cosHalfAngle = std::cos(halfAngle);

        // Normalize the axis of rotation
        vec3 axis = unit_vector(u);

        // Create and return the quaternion
        return vec4(
            axis.x() * sinHalfAngle, // x-component
            axis.y() * sinHalfAngle, // y-component
            axis.z() * sinHalfAngle, // z-component
            cosHalfAngle             // w-component
        );
    }

};

#endif