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
            rec.hit_object = object.get();
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
                rec.hit_object = object.get();
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

        bbox = Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Early exit if the ray doesn't hit the CSG node's bounding box
        if (!bounding_box().hit(r, ray_t)) {
            return false;
        }

        // Collect events from both sides
        CSGHitList leftHits, rightHits;
        collect_hits(left.get(), r, ray_t, leftHits);
        collect_hits(right.get(), r, ray_t, rightHits);

        leftHits.sort();
        rightHits.sort();

        // Merge
        std::vector<CSGHitEvent> merged = merge_hits(leftHits, rightHits);

        //pick the first intersection
        return Operation::find_valid_hit(merged, *this, rec);
    }

    //Return all boundary crossings (entries and exits) for this CSG shape.
    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        recs.clear();

        // 1) Check bounding box
        if (!bounding_box().hit(r, ray_t)) {
            return false;
        }

        // 2) Collect hits from left and right
        std::vector<hit_record> leftRecs, rightRecs;
        bool left_hit = left->hit_all(r, ray_t, leftRecs);
        bool right_hit = right->hit_all(r, ray_t, rightRecs);

        if (!left_hit && !right_hit) {
            return false;
        }

        // Convert to CSGHitLists
        CSGHitList leftHits, rightHits;
        for (const auto& lr : leftRecs) {
            leftHits.add(lr.t, lr.is_entry, left.get(), lr.normal, lr.p, lr.material);
        }
        for (const auto& rr : rightRecs) {
            rightHits.add(rr.t, rr.is_entry, right.get(), rr.normal, rr.p, rr.material);
        }

        leftHits.sort();
        rightHits.sort();

        // 3) Merge events
        std::vector<CSGHitEvent> merged = merge_hits(leftHits, rightHits);

        // 4) Walk through merged events and track "in_left" / "in_right"
        bool in_left = false;
        bool in_right = false;
        bool was_in_csg = false;  // track if we were inside the combined shape

        for (size_t i = 0; i < merged.size(); ++i) {
            const auto& m = merged[i];

            // Toggle states depending on which object generated the event
            if (m.obj == left.get()) {
                in_left ^= m.is_entry;
            }
            if (m.obj == right.get()) {
                in_right ^= m.is_entry;
            }

            // Evaluate if we are now inside the CSG shape
            bool now_in_csg = Operation::in_csg(in_left, in_right);

            // If there's a transition from outside->inside or inside->outside, record a boundary
            if (now_in_csg != was_in_csg) {
                hit_record r;
                r.t = m.t;
                r.p = m.p;
                r.material = m.material;
                r.csg_op = Operation::csg_type;
                r.is_entry = now_in_csg;  // if we're moving OUTSIDE->INSIDE, is_entry = true

                // For the normal:
                //  - If we are entering the shape, keep the object's normal as is
                //  - If we are exiting, flip the normal for consistency with outward-facing
                if (!now_in_csg) {
                    r.normal = -m.normal;
                }
                else {
                    r.normal = m.normal;
                }

                recs.push_back(r);
            }

            was_in_csg = now_in_csg;
        }

        return !recs.empty();
    }

    BoundingBox bounding_box() const override {
        return bbox;
    }

private:
    void collect_hits(const hittable* obj, const ray& r, interval ray_t, CSGHitList& hits) const {
        // Quick bounding box check
        BoundingBox obj_bbox = obj->bounding_box();
        if (!obj_bbox.hit(r, ray_t)) {
            return;
        }

        // Gather all hits from the object
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

    // Decide if we're inside the union
    static bool in_csg(bool in_left, bool in_right) {
        return in_left || in_right;
    }

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;

        for (const auto& event : events) {
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            if (is_left)  in_left ^= event.is_entry;
            if (is_right) in_right ^= event.is_entry;

            if (in_csg(in_left, in_right) && event.is_entry) {
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

    // Decide if we're inside the intersection
    static bool in_csg(bool in_left, bool in_right) {
        return in_left && in_right;
    }

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;
        bool was_in_intersection = false;

        for (const auto& event : events) {
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            if (is_left)  in_left = !in_left;
            if (is_right) in_right = !in_right;

            bool now_in_intersection = in_csg(in_left, in_right);
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

    // Decide if we're inside the difference (left minus right)
    static bool in_csg(bool in_left, bool in_right) {
        return in_left && !in_right;
    }

    template <typename NodeType>
    static bool find_valid_hit(const std::vector<CSGHitEvent>& events,
        const NodeType& node,
        hit_record& rec) {
        bool in_left = false;
        bool in_right = false;
        bool was_in_difference = false;

        for (const auto& event : events) {
            const bool is_left = (event.obj == node.left.get());
            const bool is_right = (event.obj == node.right.get());

            if (is_left)  in_left = !in_left;
            if (is_right) in_right = !in_right;

            bool now_in_difference = in_csg(in_left, in_right);
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