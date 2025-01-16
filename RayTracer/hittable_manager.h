#ifndef HITTABLE_MANAGER_H
#define HITTABLE_MANAGER_H

#include "hittable.h"
#include "hit_record.h"
#include "matrix4x4.h"
#include "boundingbox.h"
#include "bvh_node.h"
#include <unordered_map>
#include <memory>
#include <optional>
#include <unordered_set>

using std::shared_ptr;
using std::make_shared;
using ObjectID = size_t;

class HittableManager : public hittable {
public:
    HittableManager() = default;

    ObjectID add(std::shared_ptr<hittable> object, std::optional<ObjectID> manual_id = std::nullopt) {
        ObjectID id = manual_id.value_or(next_id++);

        // Check if the ID is already in use
        if (used_ids.find(id) != used_ids.end()) {
            throw std::runtime_error("Error: ObjectID " + std::to_string(id) + " already exists.");
        }

        // Register the ID as used
        used_ids.insert(id);

        // Add the object to the manager
        objects[id] = object;

        // Update next_id to ensure no overlap with manually assigned IDs
        if (manual_id && id >= next_id) {
            next_id = id + 1;
        }

        // Invalidate BVH as the objects have changed
        root_bvh = nullptr;

        return id;
    }


    void remove(ObjectID id) {
        auto it = objects.find(id);
        if (it != objects.end()) {
            objects.erase(it);          // Remove the object from the manager
            used_ids.erase(id);         // Mark the ID as no longer used

            // Invalidate BVH as the objects have changed
            root_bvh = nullptr;
        }
        else {
            std::cerr << "Warning: Attempted to remove non-existent ObjectID " << id << "\n";
        }
    }

    shared_ptr<hittable> get(ObjectID id) const {
        if (objects.find(id) != objects.end()) {
            return objects.at(id);
        }
        return nullptr;
    }

    ObjectID get_next_id() const {
        return next_id;
    }

    void clear() {
        objects.clear();
        used_ids.clear();
        root_bvh = nullptr;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (root_bvh) {
            // Use BVH for optimized hit detection
            return root_bvh->hit(r, ray_t, rec);
        }
        else {
            // Fallback to linear traversal if BVH is not built
            return defaultHitTraversal(r, ray_t, rec);
        }
    }

    BoundingBox bounding_box() const override {
        if (root_bvh) {
            return root_bvh->bounding_box();
        }
        else if (objects.empty()) {
            throw std::runtime_error("BoundingBox requested for an empty HittableManager.");
        }

        BoundingBox combined_box;
        bool first_box = true;

        for (const auto& [id, object] : objects) {
            if (first_box) {
                combined_box = object->bounding_box();
                first_box = false;
            }
            else {
                combined_box = combined_box.enclose(object->bounding_box());
            }
        }

        return combined_box;
    }

    void buildBVH() {
        // Collect all objects into a vector
        std::cout << "Building BVH \n";
        std::vector<std::shared_ptr<hittable>> object_list;
        for (const auto& [id, object] : objects) {
            object_list.push_back(object);
        }

        // Construct the BVH
        if (!object_list.empty()) {
            root_bvh = std::make_shared<BVHNode>(object_list, 0, object_list.size());
        }
    }

    void transform(const Matrix4x4& transform) override {
        std::cout << "Applying transformation to all individual objects in HittableManager:\n";
        transform.print(); // Debug the transformation matrix

        for (auto& [id, object] : objects) {
            object->transform(transform); // Transform regular objects
        }

        // Invalidate BVH after transformation
        root_bvh = nullptr;
    }

    void transform_object(ObjectID id, const Matrix4x4& transform) {
        if (objects.find(id) != objects.end()) {
            objects[id]->transform(transform);
            root_bvh = nullptr; // Invalidate BVH
        }
        else {
            throw std::runtime_error("Invalid ObjectID: " + std::to_string(id));
        }
    }

    void transform_range(ObjectID start_id, ObjectID end_id, const Matrix4x4& transform) {
        for (ObjectID id = start_id; id <= end_id; ++id) {
            try {
                transform_object(id, transform);
            }
            catch (const std::exception& e) {
                std::cerr << "Error transforming object with ID " << id << ": " << e.what() << "\n";
            }
        }
    }

private:
    ObjectID next_id = 0;
    std::unordered_map<ObjectID, std::shared_ptr<hittable>> objects;
    std::unordered_set<ObjectID> used_ids; // Track all used IDs
    std::shared_ptr<BVHNode> root_bvh = nullptr; // Root of the BVH tree

    bool defaultHitTraversal(const ray& r, interval ray_t, hit_record& rec) const {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& [id, object] : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

#endif
