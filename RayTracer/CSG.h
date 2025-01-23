#ifndef CSG_H
#define CSG_H

#include <memory>
#include "hittable.h"
#include "boundingbox.h"

// Forward declarations
struct CSGHitEvent;
class CSGHitList;

// Leaf node to wrap a primitive object
class CSGPrimitive : public hittable {
public:
    CSGPrimitive(std::shared_ptr<hittable> object) : object(object) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        bool hit = object->hit(r, ray_t, rec);
        if (hit) {
            rec.csg_op = hit_record::CSGType::NONE;
            rec.hit_object_id = reinterpret_cast<std::uintptr_t>(object.get());
        }
        return hit;
    }

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        // Delegate to the underlying object's hit_all() method
        bool hit = object->hit_all(r, ray_t, recs);

        // Update the hit records to include CSG-specific information
        if (hit) {
            for (auto& rec : recs) {
                rec.csg_op = hit_record::CSGType::NONE;
                rec.hit_object_id = reinterpret_cast<std::uintptr_t>(object.get());
            }
        }

        return hit;
    }

    BoundingBox bounding_box() const override {
        return object->bounding_box();
    }

private:
    std::shared_ptr<hittable> object;
};

// CSG Hit Event structure
struct CSGHitEvent {
    double t;
    bool is_entry;
    const hittable* obj;
    vec3 normal;
    point3 p;
    const mat* material;
};

// CSG Hit List class
class CSGHitList {
public:
    std::vector<CSGHitEvent> events;

    void add(double t, bool is_entry, const hittable* obj, const vec3& normal, const point3& p, const mat* material) {
        events.push_back({ t, is_entry, obj, normal, p, material });
    }

    void sort() {
        std::sort(events.begin(), events.end(), [](const CSGHitEvent& a, const CSGHitEvent& b) {
            return a.t < b.t;
            });
    }
};

// Base CSG node class template
template <typename Operation>
class CSGNode : public hittable {
public:
    std::shared_ptr<hittable> left, right;
    BoundingBox bbox;

    CSGNode(std::shared_ptr<hittable> left, std::shared_ptr<hittable> right)
        : left(left), right(right) {
        // Compute and cache the bounding box once
        bbox = Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Early exit if the ray doesn't hit the CSG node's bounding box
        BoundingBox bbox = this->bounding_box();
        if (!bbox.hit(r, ray_t)) {
            return false;
        }
        CSGHitList leftHits, rightHits;
        collect_hits(left.get(), r, ray_t, leftHits);
        collect_hits(right.get(), r, ray_t, rightHits);

        leftHits.sort();
        rightHits.sort();

        std::vector<CSGHitEvent> merged = merge_hits(leftHits, rightHits);
        return Operation::find_valid_hit(merged, *this, rec);
    }

    BoundingBox bounding_box() const override {
        return Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

private:
    void collect_hits(const hittable* obj, const ray& r, interval ray_t, CSGHitList& hits) const {
        BoundingBox obj_bbox = obj->bounding_box();
        if (!obj_bbox.hit(r, ray_t)) {
            return;
        }

        std::vector<hit_record> obj_hits;
        if (obj->hit_all(r, ray_t, obj_hits)) {
            for (const auto& hr : obj_hits) {
                hits.add(hr.t, hr.is_entry, obj, hr.normal, hr.p, hr.material);
            }
        }
    }

    std::vector<CSGHitEvent> merge_hits(const CSGHitList& a, const CSGHitList& b) const {
        std::vector<CSGHitEvent> merged;
        merged.reserve(a.events.size() + b.events.size());
        std::merge(a.events.begin(), a.events.end(),
            b.events.begin(), b.events.end(),
            std::back_inserter(merged),
            [](const CSGHitEvent& x, const CSGHitEvent& y) {
                return x.t < y.t;
            });
        return merged;
    }
};

// Union operation
struct Union {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::UNION;

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;

        for (const auto& event : events) {
            // Directly access left and right from the CSGNode
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            // Update state machine
            if (is_left) in_left ^= event.is_entry;
            if (is_right) in_right ^= event.is_entry;

            // Union is active when in either object
            const bool now_in_union = in_left || in_right;

            if (now_in_union && event.is_entry) {
                rec.t = event.t;
                rec.p = event.p;
                rec.normal = event.normal;
                rec.material = event.material;
                rec.csg_op = csg_type;
                return true;
            }
        }
        return false;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox.enclose(rightBox);
    }
};

// Intersection operation
struct Intersection {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::INTERSECTION;

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;
        bool was_in_intersection = false;

        for (const auto& event : events) {
            // Directly access left and right from the CSGNode
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            // Toggle state
            if (is_left) in_left = !in_left;
            if (is_right) in_right = !in_right;

            // Intersection is active only when inside both
            const bool now_in_intersection = in_left && in_right;

            // Capture the first entry into the intersection
            if (!was_in_intersection && now_in_intersection) {
                rec.t = event.t;
                rec.p = event.p;
                rec.normal = event.normal;
                rec.material = event.material;
                rec.csg_op = csg_type;
                return true;
            }

            was_in_intersection = now_in_intersection;
        }
        return false;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        if (!leftBox.intersects(rightBox)) {
            return BoundingBox();
        }
        return leftBox.from_intersect(rightBox);
    }
};

// Difference operation
struct Difference {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::DIFFERENCE;

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;
        bool was_in_difference = false;

        for (const auto& event : events) {
            // Directly access left and right from the CSGNode
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            // Toggle state
            if (is_left) in_left = !in_left;
            if (is_right) in_right = !in_right;

            // Difference is active when in left but not in right
            const bool now_in_difference = in_left && !in_right;

            // Capture transitions into the difference region
            if (!was_in_difference && now_in_difference) {
                rec.t = event.t;
                rec.p = event.p;
                rec.normal = event.normal;
                rec.material = event.material;
                rec.csg_op = csg_type;
                return true;
            }

            was_in_difference = now_in_difference;
        }
        return false;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox;
    }
};

#endif // CSG_H