#ifndef BOX_H
#define BOX_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include "primitive.h"
#include <algorithm>
#include <cmath>

class box : public hittable, public Primitive {
public:
    // Public parameters
    point3 min_corner;
    point3 v_max;
    mat material;

    // Constructor using min and max corners
    box(const point3& v_min, const point3& v_max, const mat& material)
        : min_corner(v_min), v_max(v_max), material(material) {
    }

    // Alternative constructor using center and width
    box(const point3& center, double width, const mat& material)
        : material(material) {
        vec3 half_width(width / 2.0, width / 2.0, width / 2.0);
        min_corner = center - half_width;
        v_max = center + half_width;
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
                if (origin[i] < min_corner[i] || origin[i] > v_max[i]) {
                    return false;
                }
            }
            else {
                double inv_d = 1.0 / direction[i];
                double t0 = (min_corner[i] - origin[i]) * inv_d;
                double t1 = (v_max[i] - origin[i]) * inv_d;
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
            double mid_point = 0.5 * (min_corner[hit_axis] + v_max[hit_axis]);
            outward_normal[hit_axis] = (rec.p[hit_axis] < mid_point) ? -1.0 : 1.0;
        }

        rec.set_face_normal(r, outward_normal);

        if (!rec.front_face) {
            return false;
        }

        return true;
    }

    // Axis-Aligned Instersection Test for Octree usage
    char test_bb(const BoundingBox& bb) const override {
        // Check for complete separation (no overlap)
        if (bb.vmin.x() >= v_max.x() || bb.vmax().x() <= min_corner.x() ||
            bb.vmin.y() >= v_max.y() || bb.vmax().y() <= min_corner.y() ||
            bb.vmin.z() >= v_max.z() || bb.vmax().z() <= min_corner.z()) {
            return 'w'; // Completely outside
        }

        // Check if the bounding box is completely inside this box
        if (bb.vmin.x() >= min_corner.x() && bb.vmax().x() <= v_max.x() &&
            bb.vmin.y() >= min_corner.y() && bb.vmax().y() <= v_max.y() &&
            bb.vmin.z() >= min_corner.z() && bb.vmax().z() <= v_max.z()) {
            return 'b'; // Completely inside
        }

        // Otherwise, there is partial overlap
        return 'g';
    }

};

#endif