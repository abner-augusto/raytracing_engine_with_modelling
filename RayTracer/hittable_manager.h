#ifndef HITTABLE_MANAGER_H
#define HITTABLE_MANAGER_H

#include "hittable.h"
#include "hittable_group.h"
#include "matrix4x4.h"
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

    // Add an object with an optional manually assigned ID
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

        // Update next_id if a manual ID is provided and overlaps with next_id
        if (manual_id && id >= next_id) {
            next_id = id + 1;
        }

        return id;
    }

    void remove(ObjectID id) {
        auto it = objects.find(id);
        if (it != objects.end()) {
            objects.erase(it);          // Remove the object from the manager
            used_ids.erase(id);         // Mark the ID as no longer used
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
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
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

    void transform(const Matrix4x4& transform) override {
        std::cout << "Applying transformation to all individual objects in HittableManager:\n";
        transform.print(); // Debug the transformation matrix

        for (auto& [id, object] : objects) {
            // Skip groups to avoid transforming their members here
            if (dynamic_cast<hittable_group*>(object.get())) {
                continue;
            }
            object->transform(transform); // Transform regular objects
        }
    }


    void transform_object(ObjectID id, const Matrix4x4& transform) {
        if (objects.find(id) != objects.end()) {
            objects[id]->transform(transform);
        }
        else {
            throw std::runtime_error("Invalid ObjectID: " + std::to_string(id));
        }
    }

    void transform_range(ObjectID start_id, ObjectID end_id, const Matrix4x4& transform) {
        for (ObjectID id = start_id; id <= end_id; ++id) {
            try {
                transform_object(id, transform);
                //std::cout << "Transformed object with ID " << id << ".\n";
            }
            catch (const std::exception& e) {
                std::cerr << "Error transforming object with ID " << id << ": " << e.what() << "\n";
            }
        }
    }

    ObjectID create_group(const std::vector<ObjectID>& object_ids) {
        auto group = make_shared<hittable_group>();
        for (auto id : object_ids) {
            if (objects.find(id) != objects.end()) {
                group->add(objects[id]);
            }
        }
        return add(group);
    }

private:
    ObjectID next_id = 0;
    std::unordered_map<ObjectID, std::shared_ptr<hittable>> objects;
    std::unordered_set<ObjectID> used_ids; // Track all used IDs
};


#endif
