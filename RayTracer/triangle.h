#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <algorithm>
#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"

class triangle : public hittable {
public:
    // New constructor: pass vertices and an explicit normal (without UVs)
    triangle(const point3& _v0, const point3& _v1, const point3& _v2,
        const vec3& _normal, const mat& m)
        : v0(_v0), v1(_v1), v2(_v2),
        u0(0.0), v0_uv(0.0),   // default UVs
        u1(1.0), v1_uv(0.0),
        u2(0.0), v2_uv(1.0),
        material(m),
        useCustomNormal(true),
        customNormal(unit_vector(_normal))
    {
    }

    // New constructor: pass vertices, UV coordinates, and an explicit normal
    triangle(const point3& _v0, const point3& _v1, const point3& _v2,
        const vec3& _normal,
        double _u0, double _v0_uv, double _u1, double _v1_uv, double _u2, double _v2_uv,
        const mat& m)
        : v0(_v0), v1(_v1), v2(_v2),
        u0(_u0), v0_uv(_v0_uv),
        u1(_u1), v1_uv(_v1_uv),
        u2(_u2), v2_uv(_v2_uv),
        material(m),
        useCustomNormal(true),
        customNormal(unit_vector(_normal))
    {
    }

    // Existing constructors (without custom normals)
    triangle(const point3& _v0, const point3& _v1, const point3& _v2, const mat& m)
        : v0(_v0), v1(_v1), v2(_v2),
        u0(0.0), v0_uv(0.0),
        u1(1.0), v1_uv(0.0),
        u2(0.0), v2_uv(1.0),
        material(m),
        useCustomNormal(false)
    {
    }

    triangle(const point3& _v0, const point3& _v1, const point3& _v2,
        double _u0, double _v0_uv, double _u1, double _v1_uv, double _u2, double _v2_uv,
        const mat& m)
        : v0(_v0), v1(_v1), v2(_v2),
        u0(_u0), v0_uv(_v0_uv),
        u1(_u1), v1_uv(_v1_uv),
        u2(_u2), v2_uv(_v2_uv),
        material(m),
        useCustomNormal(false)
    {
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const double epsilon = 1e-7; // Small value to avoid division by zero

        // Apply bias to the ray interval to avoid precision issues
        interval biased_ray_t = ray_t.with_bias(epsilon);

        // Calculate edges of the triangle
        const vec3 edge01 = v1 - v0;
        const vec3 edge02 = v2 - v0;

        // Compute the vector P and the determinant
        const vec3 P = cross(r.direction(), edge02);
        const double determinant = dot(edge01, P);

        // If the determinant is near zero, the ray is parallel to the triangle
        if (std::fabs(determinant) < epsilon) {
            return false;
        }

        const double invDet = 1.0 / determinant; // Inverse of the determinant
        const vec3 T = r.origin() - v0; // Vector from vertex v0 to ray origin

        // Calculate barycentric coordinate u
        const double u_bary = dot(T, P) * invDet;

        // Define a valid barycentric range and expand it for precision
        interval valid_barycentric(0.0, 1.0);
        valid_barycentric.expand(epsilon);

        // Check if u is within the valid range
        if (!valid_barycentric.contains(u_bary)) {
            return false;
        }

        // Calculate barycentric coordinate v
        const vec3 Q = cross(T, edge01);
        const double v_bary = dot(r.direction(), Q) * invDet;

        // Check if v is within the valid range
        if (!valid_barycentric.contains(v_bary)) {
            return false;
        }

        // Check if u + v is less than or equal to 1
        if (!valid_barycentric.contains(u_bary + v_bary)) {
            return false;
        }

        // Calculate the intersection point t
        const double t = dot(edge02, Q) * invDet;

        // Check if the intersection point is within the ray's valid time interval
        if (!biased_ray_t.contains(t)) {
            return false;
        }

        // Use custom normal if provided; otherwise, compute it from edges.
        vec3 normal;
        if (useCustomNormal) {
            normal = customNormal;
        }
        else {
            normal = unit_vector(cross(edge01, edge02));
        }

        // Fill the hit record with intersection information
        rec.t = t;
        rec.p = r.at(t); // Compute the hit point
        rec.normal = normal; // Set the surface normal
        rec.material = &material; // Set the material of the triangle

        // Calculate texture coordinates using barycentric coordinates
        const double w = 1.0 - u_bary - v_bary;  // Third barycentric coordinate
        rec.u = w * u0 + u_bary * u1 + v_bary * u2; // Interpolated u
        rec.v = w * v0_uv + u_bary * v1_uv + v_bary * v2_uv; // Interpolated v

        return true; // Hit occurred
    }

    void transform(const Matrix4x4& matrix) override {
        v0 = matrix.transform_point(v0);
        v1 = matrix.transform_point(v1);
        v2 = matrix.transform_point(v2);

        double det = matrix.determinant();
        if (det < 0.0) {
            std::swap(v1, v2);
            // Swap corresponding UV coordinates
            std::swap(u1, u2);
            std::swap(v1_uv, v2_uv);
        }
    }

    BoundingBox bounding_box() const override {
        // Compute the min and max coordinates for the bounding box
        point3 min_point(
            std::min({ v0.x(), v1.x(), v2.x() }),
            std::min({ v0.y(), v1.y(), v2.y() }),
            std::min({ v0.z(), v1.z(), v2.z() })
        );

        point3 max_point(
            std::max({ v0.x(), v1.x(), v2.x() }),
            std::max({ v0.y(), v1.y(), v2.y() }),
            std::max({ v0.z(), v1.z(), v2.z() })
        );

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Triangle";
    }

    void set_material(const mat& new_material) override {
        material = new_material;
    }

    mat get_material() const override {
        return material;
    }

    bool is_point_inside(const point3& p) const override {
        // Calculate edges
        vec3 edge0 = v1 - v0;
        vec3 edge1 = v2 - v1;
        vec3 edge2 = v0 - v2;

        // Calculate vectors from vertices to the point
        vec3 vp0 = p - v0;
        vec3 vp1 = p - v1;
        vec3 vp2 = p - v2;

        // Calculate cross products
        vec3 c0 = cross(edge0, vp0);
        vec3 c1 = cross(edge1, vp1);
        vec3 c2 = cross(edge2, vp2);

        // Calculate the normal of the triangle
        vec3 normal = cross(edge0, edge1);

        // Check if all cross products have the same orientation as the normal
        if (dot(normal, c0) >= 0 && dot(normal, c1) >= 0 && dot(normal, c2) >= 0) {
            return true;
        }
        return false;
    }

    // Returns:
    // 'w' if the bounding box doesn't intersect the triangle (empty)
    // 'b' if the bounding box is completely inside the triangle (full)
    // 'g' otherwise (partial)
    char test_bb(const BoundingBox& bb) const override {

        // Fast rejection: if the bounding box doesn't intersect the triangle's bounding box, return 'w'
        if (!bb.intersects(this->bounding_box())) {
            return 'w';
        }

        // Get all 8 corners of the bounding box
        std::vector<point3> corners = bb.getVertices();

        // Check if all corners are inside the triangle
        bool all_inside = true;
        for (const point3& corner : corners) {
            if (!this->is_point_inside(corner)) {
                all_inside = false;
                break;
            }
        }
        if (all_inside) {
            return 'b'; // bounding box completely inside
        }
        //Check if triangle vertices are inside the bounding box
        if (bb.contains(v0) || bb.contains(v1) || bb.contains(v2)) {
            return 'g';//partial
        }

        // If not all corners are inside, and box intersects, it's a partial intersection.
        return 'g';
    }

private:
    point3 v0, v1, v2;         // Vertices of the triangle
    double u0, v0_uv;          // UV coordinates for v0
    double u1, v1_uv;          // UV coordinates for v1
    double u2, v2_uv;          // UV coordinates for v2
    mat material;
    // New members for handling an explicit normal:
    bool useCustomNormal = false;
    vec3 customNormal;
};

#endif