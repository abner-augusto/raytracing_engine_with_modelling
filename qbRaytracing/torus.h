#ifndef TORUS_H
#define TORUS_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include <cmath>
#include <algorithm>

// Intersection routine adapted for index-based access and no vec2 class
static double torIntersect(const vec3& ro, const vec3& rd, double major_radius, double minor_radius) {
    double Ra = major_radius;
    double ra = minor_radius;
    double Ra2 = Ra * Ra;
    double ra2 = ra * ra;

    double m = dot(ro, ro);
    double n = dot(ro, rd);
    double k = (m + Ra2 - ra2) / 2.0;

    double k3 = n;
    double k2 = n * n - Ra2 * (rd[0] * rd[0] + rd[1] * rd[1]) + k;
    double k1 = n * k - Ra2 * (rd[0] * ro[0] + rd[1] * ro[1]);
    double k0 = k * k - Ra2 * (ro[0] * ro[0] + ro[1] * ro[1]);

    double po = 1.0;
    if (std::fabs(k3 * (k3 * k3 - k2) + k1) < 0.01) {
        po = -1.0;
        double tmp = k1;
        k1 = k3;
        k3 = tmp;
        k0 = 1.0 / k0;
        k1 = k1 * k0;
        k2 = k2 * k0;
        k3 = k3 * k0;
    }

    double c2 = k2 * 2.0 - 3.0 * k3 * k3;
    double c1 = k3 * (k3 * k3 - k2) + k1;
    double c0 = k3 * (k3 * (c2 + 2.0 * k2) - 8.0 * k1) + 4.0 * k0;
    c2 /= 3.0;
    c1 *= 2.0;
    c0 /= 3.0;
    double Q = c2 * c2 + c0;
    double R = c2 * c2 * c2 - 3.0 * c2 * c0 + c1 * c1;
    double h = R * R - Q * Q * Q;

    auto sign = [](double x) { return (x > 0.0) ? 1.0 : (x < 0.0 ? -1.0 : 0.0); };

    double t = -1.0;

    if (h >= 0.0) {
        double sqh = std::sqrt(h);
        double v = sign(R + sqh) * std::pow(std::fabs(R + sqh), 1.0 / 3.0);
        double u = sign(R - sqh) * std::pow(std::fabs(R - sqh), 1.0 / 3.0);
        double s[2] = { (v + u) + 4.0 * c2, (v - u) * std::sqrt(3.0) };
        double y = std::sqrt(0.5 * (std::sqrt(s[0] * s[0] + s[1] * s[1]) + s[0]));
        double x = 0.5 * s[1] / y;
        double r = 2.0 * c1 / (x * x + y * y);
        double t1 = x - r - k3;
        t1 = (po < 0.0) ? 2.0 / t1 : t1;
        double t2 = -x - r - k3;
        t2 = (po < 0.0) ? 2.0 / t2 : t2;

        double t_min = 1e20;
        if (t1 > 0.0) t_min = t1;
        if (t2 > 0.0) t_min = std::min(t_min, t2);
        t = (t_min < 1e20) ? t_min : -1.0;
    }
    else {
        double sQ = std::sqrt(Q);
        double w = sQ * std::cos(std::acos(-R / (sQ * Q)) / 3.0);
        double d2 = -(w + c2);
        if (d2 < 0.0) return -1.0;
        double d1 = std::sqrt(d2);
        double h1 = std::sqrt(w - 2.0 * c2 + c1 / d1);
        double h2 = std::sqrt(w - 2.0 * c2 - c1 / d1);

        double t1 = -d1 - h1 - k3;
        t1 = (po < 0.0) ? 2.0 / t1 : t1;
        double t2 = -d1 + h1 - k3;
        t2 = (po < 0.0) ? 2.0 / t2 : t2;
        double t3 = d1 - h2 - k3;
        t3 = (po < 0.0) ? 2.0 / t3 : t3;
        double t4 = d1 + h2 - k3;
        t4 = (po < 0.0) ? 2.0 / t4 : t4;

        double t_min = 1e20;
        if (t1 > 0.0) t_min = t1;
        if (t2 > 0.0) t_min = std::min(t_min, t2);
        if (t3 > 0.0) t_min = std::min(t_min, t3);
        if (t4 > 0.0) t_min = std::min(t_min, t4);

        t = (t_min < 1e20) ? t_min : -1.0;
    }

    return t;
}

static vec3 torNormal(const vec3& pos, double major_radius, double minor_radius) {
    double R = major_radius;
    double r = minor_radius;

    double X = pos[0];
    double Y = pos[1];
    double Z = pos[2];

    double d = std::sqrt(X * X + Y * Y);

    if (d < 1e-14) {
        return unit_vector(vec3(X, Y, Z));
    }

    double factor = (d - R) / d;

    vec3 grad(
        2.0 * (d - R) * (X / d),
        2.0 * (d - R) * (Y / d),
        2.0 * Z
    );

    return unit_vector(grad);
}

class torus : public hittable {
public:
    torus(const point3& center, double major_radius, double minor_radius, const vec3& axis_direction, const mat& material)
        : center(center),
        major_radius(std::fmax(0.0, major_radius)),
        minor_radius(std::fmax(0.0, minor_radius)),
        material(material) {
        w = unit_vector(axis_direction);
        vec3 arbitrary = (std::fabs(w[0]) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u = unit_vector(cross(arbitrary, w));
        v = cross(w, u);
    }


    void set_center(const point3& c) {
        center = c;
    }

    void set_major_radius(double R) {
        major_radius = std::fmax(0.0, R);
    }

    void set_minor_radius(double r) {
        minor_radius = std::fmax(0.0, r);
    }

    void set_axis_direction(const vec3& dir) {
        w = unit_vector(dir);
        vec3 arbitrary = (std::fabs(w[0]) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u = unit_vector(cross(arbitrary, w));
        v = cross(w, u);
    }

    void set_material(const mat& m) {
        material = m;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 ro = r.origin() - center;
        vec3 rd = r.direction();

        vec3 ro_local(dot(ro, u), dot(ro, v), dot(ro, w));
        vec3 rd_local(dot(rd, u), dot(rd, v), dot(rd, w));

        double t = torIntersect(ro_local, rd_local, major_radius, minor_radius);
        if (t < 0.0 || !ray_t.contains(t))
            return false;

        rec.t = t;
        rec.p = r.at(t);

        vec3 p_local = ro_local + t * rd_local;

        vec3 n_local = torNormal(p_local, major_radius, minor_radius);

        vec3 n_world = n_local[0] * u + n_local[1] * v + n_local[2] * w;

        rec.set_face_normal(r, n_world);
        rec.material = &material;

        return true;
    }

private:
    point3 center;
    double major_radius;
    double minor_radius;
    mat material;

    vec3 u, v, w;
};

#endif
