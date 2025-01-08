#ifndef BOX_H
#define BOX_H

#include <algorithm>
#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "boundingbox.h"
#include "primitive.h"

class box : public hittable, public Primitive {
public:

    std::vector<point3> vertices;

    mat material;

    // Constructor using min and max corners
    box(const point3& v_min, const point3& v_max, const mat& material)
        : material(material) {
        initialize_vertices(v_min, v_max);
    }

    // Alternative constructor using center and width
    box(const point3& center, double width, const mat& material)
        : material(material) {
        vec3 half_width(width / 2.0, width / 2.0, width / 2.0);
        initialize_vertices(center - half_width, center + half_width);
    }

    // Constructor using min corner and dimensions
    box(const point3& v_min, double length, double width, double height, const mat& material)
        : material(material) {
        initialize_vertices(v_min, v_min + point3(length, width, height));
    }

    // Get the box's minimum corner
    point3 get_min_corner() const { return vertices[0]; }

    // Get the box's maximum corner
    point3 get_max_corner() const {
        point3 v_max = vertices[0];
        for (const auto& v : vertices) {
            v_max = max(v_max, v);
        }
        return v_max;
    }

    // Get an individual vertex
    point3 get_vertex(int index) const {
        if (index >= 0 && index < vertices.size()) {
            return vertices[index];
        }
        throw std::out_of_range("Invalid vertex index");
    }

    // Apply a transformation matrix to all vertices
    void transform(const Matrix4x4& matrix) override {
        for (auto& vertex : vertices) {
            vertex = matrix.transform_point(vertex);
        }
        update_bounds();
    }

    // Hit function
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const vec3& origin = r.origin();
        const vec3& direction = r.direction();

        double t_min = ray_t.min;
        double t_max = ray_t.max;
        int hit_axis = -1;

        for (int i = 0; i < 3; ++i) {
            if (std::fabs(direction[i]) < 1e-12) {
                if (origin[i] < vertices[0][i] || origin[i] > vertices[6][i]) {
                    return false;
                }
            }
            else {
                double inv_d = 1.0 / direction[i];
                double t0 = (vertices[0][i] - origin[i]) * inv_d;
                double t1 = (vertices[6][i] - origin[i]) * inv_d;
                if (t0 > t1) std::swap(t0, t1);

                if (t0 > t_min) {
                    t_min = t0;
                    hit_axis = i;
                }
                t_max = (t1 < t_max) ? t1 : t_max;

                if (t_max <= t_min) {
                    return false;
                }
            }
        }

        rec.t = t_min;
        rec.p = r.at(t_min);
        rec.material = &material;

        vec3 outward_normal(0, 0, 0);
        if (hit_axis >= 0) {
            double mid_point = 0.5 * (vertices[0][hit_axis] + vertices[6][hit_axis]);
            outward_normal[hit_axis] = (rec.p[hit_axis] < mid_point) ? -1.0 : 1.0;
        }

        rec.set_face_normal(r, outward_normal);

        if (!rec.front_face) {
            return false;
        }

        return true;
    }

    // Axis-Aligned Intersection Test for Octree usage
    char test_bb(const BoundingBox& bb) const override {
        // Check for complete separation (no overlap)
        if (bb.vmin.x() >= vertices[6].x() || bb.vmax().x() <= vertices[0].x() ||
            bb.vmin.y() >= vertices[6].y() || bb.vmax().y() <= vertices[0].y() ||
            bb.vmin.z() >= vertices[6].z() || bb.vmax().z() <= vertices[0].z()) {
            return 'w'; // Completely outside
        }

        // Check if the bounding box is completely inside this box
        if (bb.vmin.x() >= vertices[0].x() && bb.vmax().x() <= vertices[6].x() &&
            bb.vmin.y() >= vertices[0].y() && bb.vmax().y() <= vertices[6].y() &&
            bb.vmin.z() >= vertices[0].z() && bb.vmax().z() <= vertices[6].z()) {
            return 'b'; // Completely inside
        }

        // Otherwise, there is partial overlap
        return 'g';
    }

private:
    // Initialize the vertices based on min and max corners
    void initialize_vertices(const point3& v_min, const point3& v_max) {
        vertices = {
            v_min,
            point3(v_max.x(), v_min.y(), v_min.z()),
            point3(v_max.x(), v_max.y(), v_min.z()),
            point3(v_min.x(), v_max.y(), v_min.z()),
            point3(v_min.x(), v_min.y(), v_max.z()),
            point3(v_max.x(), v_min.y(), v_max.z()),
            point3(v_max.x(), v_max.y(), v_max.z()),
            point3(v_min.x(), v_max.y(), v_max.z()),
        };
    }

    // Update bounds based on current vertices
    void update_bounds() {
        point3 v_min = vertices[0];
        point3 v_max = vertices[0];

        for (const auto& vertex : vertices) {
            v_min = min(v_min, vertex);
            v_max = max(v_max, vertex);
        }

        // Update min_corner and v_max
        vertices[0] = v_min;
        vertices[6] = v_max;
    }
};

#endif