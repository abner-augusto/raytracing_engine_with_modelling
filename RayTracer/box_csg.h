#ifndef BOX_CSG_H
#define BOX_CSG_H

#include <algorithm>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "boundingbox.h"

class box_csg : public hittable {
public:
    box_csg(const point3& min_corner, const point3& max_corner, const mat& material)
        : min_corner(min_corner), max_corner(max_corner), material(material) {
        update_bounds();
    }

    box_csg(const point3& center, double width, const mat& material)
        : material(material) {
        vec3 half_size = vec3::fill(width * 0.5);
        min_corner = center - half_size;
        max_corner = center + half_size;
        update_bounds();
    }

    void set_min_corner(const point3& new_min_corner) {
        min_corner = new_min_corner;
        update_bounds();
    }

    void set_max_corner(const point3& new_max_corner) {
        max_corner = new_max_corner;
        update_bounds();
    }

    void set_material(const mat& new_material) override {
        material = new_material;
    }

    mat get_material() const override {
        return material;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 invD = vec3::fill(1.0) / r.direction();
        vec3 t0 = (min_corner - r.origin()) * invD;
        vec3 t1 = (max_corner - r.origin()) * invD;

        vec3 t_min = min(t0, t1);
        vec3 t_max = max(t0, t1);

        double tNear = std::max({ t_min.x(), t_min.y(), t_min.z() });
        double tFar = std::min({ t_max.x(), t_max.y(), t_max.z() });

        if (tNear > tFar || tFar < 0.0) return false;

        rec.t = (tNear > 0.0) ? tNear : tFar;
        rec.p = r.at(rec.t);

        if (tNear > 0.0) {
            if (tNear == t_min.x()) rec.normal = vec3(-1, 0, 0);
            else if (tNear == t_min.y()) rec.normal = vec3(0, -1, 0);
            else rec.normal = vec3(0, 0, -1);
        }
        else {
            if (tFar == t_max.x()) rec.normal = vec3(1, 0, 0);
            else if (tFar == t_max.y()) rec.normal = vec3(0, 1, 0);
            else rec.normal = vec3(0, 0, 1);
        }

        rec.normal *= -sign(r.direction());
        rec.set_face_normal(r, rec.normal);
        rec.material = &material;
        rec.hit_object = this;
        return true;
    }
    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override
    {
        out_intersections.clear();

        vec3 invD = vec3::fill(1.0) / r.direction();
        vec3 t0 = (min_corner - r.origin()) * invD;
        vec3 t1 = (max_corner - r.origin()) * invD;

        vec3 t_min = min(t0, t1);
        vec3 t_max = max(t0, t1);

        double tNear = std::max({ t_min.x(), t_min.y(), t_min.z() });
        double tFar = std::min({ t_max.x(), t_max.y(), t_max.z() });

        // Basic rejection
        if (tNear > tFar || tFar < ray_t.min || tNear > ray_t.max) {
            return false;
        }

        // Are we starting inside the box?
        bool ray_starts_inside = is_point_inside(r.origin());

        // 1) Handle the "near" intersection if it's within [ray_t.min, ray_t.max]
        if (tNear >= ray_t.min && tNear <= ray_t.max) {
            point3 p = r.at(tNear);

            // Pick the raw normal for tNear
            vec3 normal;
            if (tNear == t_min.x()) normal = vec3(-1, 0, 0);
            else if (tNear == t_min.y()) normal = vec3(0, -1, 0);
            else                         normal = vec3(0, 0, -1);

            // Match the flipping logic in Box::hit()
            normal *= -sign(r.direction());

            // (Optional) make sure it faces "opposite" the ray if you want a front-face style:
            if (dot(r.direction(), normal) > 0) {
                normal = -normal;
            }

            // If we start outside, crossing tNear is an entry event
            bool is_entry = !ray_starts_inside;

            // Add it
            out_intersections.emplace_back(tNear, is_entry, this, normal, p);
        }

        // 2) Handle the "far" intersection if it's within [ray_t.min, ray_t.max]
        if (tFar >= ray_t.min && tFar <= ray_t.max) {
            point3 p = r.at(tFar);

            // Pick the raw normal for tFar
            vec3 normal;
            if (tFar == t_max.x()) normal = vec3(1, 0, 0);
            else if (tFar == t_max.y()) normal = vec3(0, 1, 0);
            else                        normal = vec3(0, 0, 1);

            // Match the flipping logic in Box::hit()
            normal *= -sign(r.direction());

            // (Optional) front-face test
            if (dot(r.direction(), normal) > 0) {
                normal = -normal;
            }

            bool is_entry = ray_starts_inside;

            out_intersections.emplace_back(tFar, is_entry, this, normal, p);
        }

        return !out_intersections.empty();
    }


    std::string get_type_name() const override {
        return "Box";
    }

    char test_bb(const BoundingBox& bb) const override {
        // Quick test - if the test box is completely outside our bounds in any dimension,
        // then it must be completely outside
        if (bb.vmax.x() < min_corner.x() || bb.vmin.x() > max_corner.x() ||
            bb.vmax.y() < min_corner.y() || bb.vmin.y() > max_corner.y() ||
            bb.vmax.z() < min_corner.z() || bb.vmin.z() > max_corner.z()) {
            return 'w';
        }

        // If the test box is completely inside our bounds in all dimensions,
        // then it must be completely inside
        if (bb.vmin.x() >= min_corner.x() && bb.vmax.x() <= max_corner.x() &&
            bb.vmin.y() >= min_corner.y() && bb.vmax.y() <= max_corner.y() &&
            bb.vmin.z() >= min_corner.z() && bb.vmax.z() <= max_corner.z()) {
            return 'b';
        }

        // If we get here, the box must be partially inside
        return 'g';
    }

    bool is_point_inside(const point3& p) const override {
        return (p.x() >= min_corner.x() && p.x() <= max_corner.x()) &&
            (p.y() >= min_corner.y() && p.y() <= max_corner.y()) &&
            (p.z() >= min_corner.z() && p.z() <= max_corner.z());
    }

    void transform(const Matrix4x4& matrix) override {
        min_corner = matrix.transform_point(min_corner);
        max_corner = matrix.transform_point(max_corner);
        update_bounds();
    }

    BoundingBox bounding_box() const override {
        return BoundingBox(min_corner, max_corner);
    }

private:
    point3 min_corner, max_corner;
    mat material;

    void update_bounds() {
        for (int i = 0; i < 3; i++) {
            if (min_corner[i] > max_corner[i]) {
                std::swap(min_corner[i], max_corner[i]);
            }
        }
    }
};

#endif //BOX_CSG_H
