#ifndef CSG_H
#define CSG_H

#include <memory>
#include "hittable.h"
#include "boundingbox.h"

// Base CSG node class template
template <typename Operation>
class CSGNode : public hittable {
public:
    CSGNode(std::shared_ptr<hittable> left, std::shared_ptr<hittable> right)
        : left(left), right(right) {
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record leftRec, rightRec;
        bool hitLeft = left->hit(r, ray_t, leftRec);
        bool hitRight = right->hit(r, ray_t, rightRec);

        if (Operation::apply(hitLeft, leftRec, hitRight, rightRec, rec)) {
            rec.csg_op = Operation::csg_type;
            return true;
        }
        return false;
    }

    BoundingBox bounding_box() const override {
        return Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

private:
    std::shared_ptr<hittable> left, right;
};

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

    BoundingBox bounding_box() const override {
        return object->bounding_box();
    }

private:
    std::shared_ptr<hittable> object;
};

// Templated operation implementations

// Union operation
struct Union {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::UNION;

    static bool apply(bool hitLeft, const hit_record& leftRec, bool hitRight, const hit_record& rightRec, hit_record& rec) {
        if (!hitLeft && !hitRight) return false;
        if (hitLeft && hitRight)
            rec = (leftRec.t < rightRec.t) ? leftRec : rightRec;
        else if (hitLeft)
            rec = leftRec;
        else
            rec = rightRec;
        rec.csg_op = csg_type;
        return true;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox.enclose(rightBox);
    }
};

// Intersection operation
struct Intersection {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::INTERSECTION;

    static bool apply(bool hitLeft, const hit_record& leftRec, bool hitRight, const hit_record& rightRec, hit_record& rec) {
        if (hitLeft && hitRight) {
            rec = (leftRec.t > rightRec.t) ? leftRec : rightRec;
            rec.csg_op = csg_type;
            return true;
        }
        return false;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        if (!leftBox.intersects(rightBox)) {
            return BoundingBox(); // Return an empty or default bounding box when no intersection
        }

        return leftBox.enclose(rightBox);
    }
};

// Difference operation
struct Difference {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::DIFFERENCE;

    static bool apply(bool hitLeft, const hit_record& leftRec, bool hitRight, const hit_record& rightRec, hit_record& rec) {
        if (hitLeft && !hitRight) {
            rec = leftRec;
            rec.csg_op = csg_type;
            return true;
        }
        return false;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox;
    }
};



#endif // CSG_H
