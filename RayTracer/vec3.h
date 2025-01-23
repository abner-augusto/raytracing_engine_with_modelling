#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>
#include <algorithm>

class vec3 {
public:
    double e[3];

    // Constructors
    vec3() : e{ 0, 0, 0 } {}
    vec3(double e0, double e1, double e2) : e{ e0, e1, e2 } {}

    // Static function to create a vec3 with all components set to the same value
    static vec3 fill(double value) {
        return vec3(value, value, value);
    }

    // Accessors
    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](size_t i) const { return e[i]; }
    double& operator[](size_t i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(double t) {
        return *this *= 1 / t;
    }

    vec3& operator*=(const vec3& v) {
        e[0] *= v.e[0];
        e[1] *= v.e[1];
        e[2] *= v.e[2];
        return *this;
    }

    vec3& operator/=(const vec3& v) {
        e[0] /= v.e[0];
        e[1] /= v.e[1];
        e[2] /= v.e[2];
        return *this;
    }

    bool operator==(const vec3& other) const {
        const double epsilon = 1e-8;
        return std::fabs(e[0] - other.e[0]) < epsilon &&
            std::fabs(e[1] - other.e[1]) < epsilon &&
            std::fabs(e[2] - other.e[2]) < epsilon;
    }

    bool operator!=(const vec3& other) const {
        return !(*this == other);
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    vec3 abs() const {
        return vec3(std::abs(e[0]), std::abs(e[1]), std::abs(e[2]));
    }

    vec3 cmax(const vec3& v) const {
        return vec3(std::max(e[0], v.e[0]), std::max(e[1], v.e[1]), std::max(e[2], v.e[2]));
    }

    vec3 cmin(const vec3& v) const {
        return vec3(std::min(e[0], v.e[0]), std::min(e[1], v.e[1]), std::min(e[2], v.e[2]));
    }

    double max() const {
        return std::max(std::max(e[0], e[1]), e[2]);
    }

    double min() const {
        return std::min(std::min(e[0], e[1]), e[2]);
    }

    vec3 inverse() const {
        return vec3(1.0 / e[0], 1.0 / e[1], 1.0 / e[2]);
    }
};

// Alias for geometric clarity
using point3 = vec3;

// Vector Utility Functions
inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

// Component-wise multiplication
inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

// Scalar multiplication
inline vec3 operator*(double t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3& v, double t) {
    return t * v;
}

// Component-wise division
inline vec3 operator/(const vec3& u, const vec3& v) {
    return vec3(u.e[0] / v.e[0], u.e[1] / v.e[1], u.e[2] / v.e[2]);
}

// Scalar division
inline vec3 operator/(const vec3& v, double t) {
    return (1 / t) * v;
}

inline vec3 operator/(double t, const vec3& v) {
    return vec3(t / v.e[0], t / v.e[1], t / v.e[2]);
}

inline double dot(const vec3& u, const vec3& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
        u.e[2] * v.e[0] - u.e[0] * v.e[2],
        u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}

inline vec3 min(const vec3& u, const vec3& v) {
    return vec3(std::min(u.e[0], v.e[0]), std::min(u.e[1], v.e[1]), std::min(u.e[2], v.e[2]));
}

inline vec3 max(const vec3& u, const vec3& v) {
    return vec3(std::max(u.e[0], v.e[0]), std::max(u.e[1], v.e[1]), std::max(u.e[2], v.e[2]));
}

inline vec3 step(const vec3& edge, const vec3& v) {
    return vec3(v.e[0] >= edge.e[0] ? 1.0 : 0.0,
        v.e[1] >= edge.e[1] ? 1.0 : 0.0,
        v.e[2] >= edge.e[2] ? 1.0 : 0.0);
}

inline vec3 sign(const vec3& v) {
    return vec3((v.e[0] > 0) - (v.e[0] < 0),
        (v.e[1] > 0) - (v.e[1] < 0),
        (v.e[2] > 0) - (v.e[2] < 0));
}

inline vec3 reflect(const vec3& I, const vec3& N) {
    return I - 2 * dot(I, N) * N;
}

inline double distance(const vec3& a, const vec3& b) {
    return (a - b).length();
}

inline double norm(const vec3& v) {
    return v.length();
}

#endif