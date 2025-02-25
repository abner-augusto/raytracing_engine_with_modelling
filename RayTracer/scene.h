#ifndef SCENE_H
#define SCENE_H

#include <unordered_map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <iostream>
#include <vector>
#include <string>
#include "hittable.h"
#include "hit_record.h"
#include "matrix4x4.h"
#include "boundingbox.h"
#include "octree.h"
#include "bvh_node.h"
#include "light.h"

using std::shared_ptr;
using std::make_shared;
using ObjectID = size_t;

class SceneManager : public hittable {
public:
    SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;


    // ------------------------------------------------------------------
    //                          Object Management
    // ------------------------------------------------------------------

    // Adds an object to the scene. Optionally, you can specify a manual ID.
    ObjectID add(std::shared_ptr<hittable> object, std::optional<ObjectID> manual_id = std::nullopt) {
        ObjectID id = manual_id.value_or(next_id++);

        // Check if the ID is already in use.
        if (used_ids.find(id) != used_ids.end()) {
            throw std::runtime_error("Error: ObjectID " + std::to_string(id) + " already exists.");
        }

        // Register the ID as used and add the object.
        used_ids.insert(id);
        objects[id] = object;

        // Update next_id to ensure no overlap with manually assigned IDs.
        if (manual_id && id >= next_id) {
            next_id = id + 1;
        }

        // Invalidate BVH as the objects have changed.
        root_bvh = nullptr;
        return id;
    }

    // Removes the object with the specified ID.
    // Also removes any octree associated with the object.
    void remove(ObjectID id) {
        auto it = objects.find(id);
        if (it != objects.end()) {
            objects.erase(it);          // Remove the object.
            used_ids.erase(id);           // Mark the ID as no longer used.
            // Remove the associated octree if it exists.
            if (octrees.find(id) != octrees.end()) {
                octrees.erase(id);
            }
            root_bvh = nullptr;         // Invalidate BVH.
        }
        else {
            std::cerr << "Warning: Attempted to remove non-existent ObjectID " << id << "\n";
        }
    }

    // Returns the object associated with the given ID.
    shared_ptr<hittable> get(ObjectID id) const {
        if (objects.find(id) != objects.end()) {
            return objects.at(id);
        }
        return nullptr;
    }

    // Return all objects as a vector.
    std::vector<std::shared_ptr<hittable>> getObjects() const {
        std::vector<std::shared_ptr<hittable>> objs;
        for (const auto& [id, object] : objects) {
            objs.push_back(object);
        }
        return objs;
    }

    // Returns the ObjectID for a given object if it exists in the scene.
    std::optional<ObjectID> get_object_id(const std::shared_ptr<hittable>& object) const {
        for (const auto& [id, obj] : objects) {
            if (obj == object) {
                return id;
            }
        }
        return std::nullopt;
    }

    ObjectID get_next_id() const {
        return next_id;
    }

    // Checks if an object with the given ID exists.
    bool contains(ObjectID id) const {
        return objects.find(id) != objects.end();
    }

    // Clears all objects and their octrees.
    void clear() {
        objects.clear();
        used_ids.clear();
        octrees.clear();
        root_bvh = nullptr;
    }

    // Returns a list of (ObjectID, name) pairs for all objects in the scene.
    std::vector<std::pair<ObjectID, std::string>> list_object_names() const {
        std::vector<std::pair<ObjectID, std::string>> result;
        for (const auto& [id, object] : objects) {
            std::string name = object->get_type_name() + " (" + std::to_string(id) + ")";
            result.push_back({ id, name });
        }
        return result;
    }

    // Applies a transformation to all objects and lights.
      // If an object has an associated octree, that octree is updated.
    void transform(const Matrix4x4& transform) override {
        std::cout << "Applying transformation to all objects in SceneManager:\n";
        transform.print();
        for (auto& [id, object] : objects) {
            object->transform(transform);
            if (octrees.find(id) != octrees.end()) {
                BoundingBox bb = object->bounding_box();
                Octree tree = Octree::FromObject(bb, *object, 3);
                octrees[id] = tree;
            }
        }
        transform_lights(transform);
        // Invalidate BVH after transformation.
        root_bvh = nullptr;
    }

    // Applies a transformation to a specific object.
    // Updates its associated octree only if one exists.
    void transform_object(ObjectID id, const Matrix4x4& transform) {
        if (objects.find(id) != objects.end()) {
            objects[id]->transform(transform);
            if (octrees.find(id) != octrees.end()) {
                BoundingBox bb = objects[id]->bounding_box();
                Octree tree = Octree::FromObject(bb, *objects[id], 3);
                octrees[id] = tree;
            }
            root_bvh = nullptr;
        }
        else {
            throw std::runtime_error("Invalid ObjectID: " + std::to_string(id));
        }
    }

    // Applies a transformation to a range of objects.
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


    // ------------------------------------------------------------------
    //                          Light Management
    // ------------------------------------------------------------------

     // Convenience methods for adding specific light types
    void add_point_light(const vec3& pos, double intensity, const color& col) {
        lights.push_back(std::make_unique<PointLight>(pos, intensity, col));
    }

    void add_directional_light(const vec3& dir, double intensity, const color& col) {
        lights.push_back(std::make_unique<DirectionalLight>(dir, intensity, col));
    }

    void add_spot_light(const vec3& pos, const vec3& dir, double intensity,
        const color& col, double cutoff, double outer_cutoff) {
        lights.push_back(std::make_unique<SpotLight>(pos, dir, intensity, col, cutoff, outer_cutoff));
    }

    void transform_lights(const Matrix4x4& matrix) {
        for (auto& light : lights) {
            light->transform(matrix);
        }
    }

    void remove_light(size_t index) {
        if (index < lights.size()) {
            lights.erase(lights.begin() + index);
        }
    }

    const std::vector<std::unique_ptr<Light>>& get_lights() const {
        return lights;
    }

    // ------------------------------------------------------------------
    //                          Octree Management
    // ------------------------------------------------------------------

    // Generates an octree for the object with the given ObjectID using its bounding box.
    void generateObjectOctree(ObjectID id, int octreeDepthLimit = 3) {
        auto it = objects.find(id);
        if (it != objects.end()) {
            BoundingBox bb = it->second->bounding_box();
            Octree tree = Octree::FromObject(bb, *it->second, octreeDepthLimit);
            octrees[id] = tree;
        }
        else {
            throw std::runtime_error("Invalid ObjectID: " + std::to_string(id));
        }
    }

    // Retrieve the filled bounding boxes for a specific object's octree.
    std::vector<BoundingBox> getOctreeFilledBoundingBoxes(ObjectID id) const {
        auto it = octrees.find(id);
        if (it != octrees.end()) {
            return it->second.GetFilledBoundingBoxes();
        }
        return {};
    }

    // Retrieve the filled bounding boxes from all octrees, useful for wireframe rendering.
    std::vector<BoundingBox> getAllOctreeFilledBoundingBoxes() const {
        std::vector<BoundingBox> allBoxes;
        for (const auto& [id, tree] : octrees) {
            std::vector<BoundingBox> boxes = tree.GetFilledBoundingBoxes();
            allBoxes.insert(allBoxes.end(), boxes.begin(), boxes.end());
        }
        return allBoxes;
    }

    bool hasOctree(ObjectID id) const {
        return octrees.find(id) != octrees.end();
    }

    const Octree& getOctree(ObjectID id) const {
        auto it = octrees.find(id);
        if (it != octrees.end()) {
            return it->second;
        }
        throw std::runtime_error("No Octree found for Object ID: " + std::to_string(id));
    }


    // ------------------------------------------------------------------
    //                          BVH Management
    // ------------------------------------------------------------------

    void buildBVH(bool log = true) {
        // Collect all objects into a vector.
        if (log) {
            std::cout << "Building BVH \n";
        }

        std::vector<shared_ptr<hittable>> object_list;
        for (const auto& [id, object] : objects) {
            object_list.push_back(object);
        }
        // Construct the BVH.
        if (!object_list.empty()) {
            root_bvh = std::make_shared<BVHNode>(object_list, 0, object_list.size());
        }
    }


    // ------------------------------------------------------------------
    //                     Hittable Interface Implementation
    // ------------------------------------------------------------------
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (root_bvh) {
            // Use BVH for optimized hit detection.
            return root_bvh->hit(r, ray_t, rec);
        }
        else {
            // Fallback to linear traversal if BVH is not built.
            return defaultHitTraversal(r, ray_t, rec);
        }
    }

    BoundingBox bounding_box() const override {
        if (root_bvh) {
            return root_bvh->bounding_box();
        }
        else if (objects.empty()) {
            throw std::runtime_error("BoundingBox requested for an empty SceneManager.");
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

private:

    // ------------------------------------------------------------------
    //                          Private Members
    // ------------------------------------------------------------------
    ObjectID next_id = 0;
    std::unordered_map<ObjectID, shared_ptr<hittable>> objects;
    std::vector<std::unique_ptr<Light>> lights;
    std::unordered_set<ObjectID> used_ids; // Track all used IDs.
    shared_ptr<BVHNode> root_bvh = nullptr;  // Root of the BVH tree.
    std::unordered_map<ObjectID, Octree> octrees; // Maps each object ID to its corresponding octree.

    // ------------------------------------------------------------------
    //                        Private Helper Functions
    // ------------------------------------------------------------------

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