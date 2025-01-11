#ifndef HITTABLE_GROUP_H
#define HITTABLE_GROUP_H

#include "hittable.h"
#include "matrix4x4.h"
#include <vector>
#include <memory>

class hittable_group : public hittable {
public:
    hittable_group() = default;

    void add(const shared_ptr<hittable>& object) {
        objects.push_back(object);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }

    void transform(const Matrix4x4& transform) override {
        for (auto& obj : objects) {
            obj->transform(transform);
        }
    }

private:
    std::vector<std::shared_ptr<hittable>> objects;
};

#endif
