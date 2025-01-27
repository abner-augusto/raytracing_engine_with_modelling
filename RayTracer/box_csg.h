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

    void set_material(const mat& new_material) {
        material = new_material;
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
        return true;
    }

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        recs.clear();
        recs.reserve(2);  // Reserve space for 2 intersections (entry and exit)

        vec3 invD = vec3::fill(1.0) / r.direction();
        vec3 t0 = (min_corner - r.origin()) * invD;
        vec3 t1 = (max_corner - r.origin()) * invD;

        vec3 t_min = min(t0, t1);
        vec3 t_max = max(t0, t1);

        double tNear = std::max({ t_min.x(), t_min.y(), t_min.z() });
        double tFar = std::min({ t_max.x(), t_max.y(), t_max.z() });

        if (tNear > tFar || tFar < ray_t.min || tNear > ray_t.max) {
            return false;
        }

        // Entry hit processing
        if (tNear >= ray_t.min && tNear <= ray_t.max) {
            hit_record entry_rec;
            entry_rec.t = tNear;
            entry_rec.p = r.at(entry_rec.t);
            entry_rec.normal = (tNear == t_min.x()) ? vec3(-1, 0, 0) :
                (tNear == t_min.y()) ? vec3(0, -1, 0) : vec3(0, 0, -1);

            entry_rec.set_face_normal(r, entry_rec.normal);
            entry_rec.material = &material;
            recs.push_back(entry_rec);
        }

        // Exit hit processing
        if (tFar >= ray_t.min && tFar <= ray_t.max) {
            hit_record exit_rec;
            exit_rec.t = tFar;
            exit_rec.p = r.at(exit_rec.t);
            exit_rec.normal = (tFar == t_max.x()) ? vec3(1, 0, 0) :
                (tFar == t_max.y()) ? vec3(0, 1, 0) : vec3(0, 0, 1);

            exit_rec.set_face_normal(r, exit_rec.normal);
            exit_rec.material = &material;
            recs.push_back(exit_rec);
        }

        return !recs.empty();
    }

    std::string get_type_name() const override {
        return "Box";
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
