#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "boundingbox.h"
#include "hittable.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

constexpr size_t LEAF_SIZE_THRESHOLD = 4;

class BVHNode : public hittable {
private:
    std::vector<std::shared_ptr<hittable>> leaf_objects;  // Store multiple objects in leaf nodes
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    BoundingBox box;
    bool is_leaf;

    // Determine optimal split axis using parallel reduction
    static size_t determineSplitAxis(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
        std::vector<double> max_dims(3, 0.0);

#pragma omp parallel
        {
            std::vector<double> local_max_dims(3, 0.0);

#pragma omp for nowait
            for (long long i = start; i < static_cast<long long>(end); ++i) {
                auto dimensions = objects[i]->bounding_box().getDimensions();
                for (int j = 0; j < 3; ++j) {
                    local_max_dims[j] = std::max(local_max_dims[j], dimensions[j]);
                }
            }

#pragma omp critical
            for (int j = 0; j < 3; ++j) {
                max_dims[j] = std::max(max_dims[j], local_max_dims[j]);
            }
        }

        return std::max_element(max_dims.begin(), max_dims.end()) - max_dims.begin();
    }

    // Build a subtree with OpenMP parallelization
    static std::shared_ptr<BVHNode> buildSubtree(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end, int depth = 0) {
        auto node = std::make_shared<BVHNode>();

        // Create a leaf node if number of objects is below threshold
        if (end - start <= LEAF_SIZE_THRESHOLD) {
            node->buildLeafNode(objects, start, end);
            return node;
        }

        size_t axis = determineSplitAxis(objects, start, end);
        size_t mid = start + (end - start) / 2;

        // Sort objects along the selected axis
        auto comparator = [axis](const std::shared_ptr<hittable>& a, const std::shared_ptr<hittable>& b) {
            return a->bounding_box().getCenter()[axis] < b->bounding_box().getCenter()[axis];
            };

        // Parallel sort for large enough chunks
        if (end - start > 1000) {
#pragma omp parallel
            {
#pragma omp single
                std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end, comparator);
            }
        }
        else {
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end, comparator);
        }

        // Dynamic parallel task creation based on available resources
#pragma omp parallel sections if(depth < omp_get_max_threads() && (end - start) > 1000)
        {
#pragma omp section
            {
                node->left = buildSubtree(objects, start, mid, depth + 1);
            }

#pragma omp section
            {
                node->right = buildSubtree(objects, mid, end, depth + 1);
            }
        }

        // Sequential fallback if parallel sections weren't used
        if (!node->left) {
            node->left = buildSubtree(objects, start, mid, depth + 1);
        }
        if (!node->right) {
            node->right = buildSubtree(objects, mid, end, depth + 1);
        }

        node->is_leaf = false;
        node->box = node->left->bounding_box().enclose(node->right->bounding_box());
        return node;
    }

    void buildLeafNode(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
        is_leaf = true;
        leaf_objects.clear();

        // Copy all objects in range to leaf node
        for (size_t i = start; i < end; ++i) {
            leaf_objects.push_back(objects[i]);
        }

        // Compute bounding box for all objects
        box = leaf_objects[0]->bounding_box();
        for (size_t i = 1; i < leaf_objects.size(); ++i) {
            box = box.enclose(leaf_objects[i]->bounding_box());
        }

        left = right = nullptr;  // Leaf nodes don't have children
    }

public:
    BVHNode() : is_leaf(false) {}

    BVHNode(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) : is_leaf(false) {
        // Set optimal number of threads based on hardware
        int num_threads = omp_get_max_threads();
        omp_set_dynamic(1);  // Enable dynamic adjustment of threads

        // Initialize parallel environment
#pragma omp parallel num_threads(num_threads)
        {
#pragma omp single
            {
                auto root = buildSubtree(objects, start, end);
                left = root->left;
                right = root->right;
                box = root->box;
                is_leaf = root->is_leaf;
                leaf_objects = root->leaf_objects;
            }
        }
    }

    virtual ~BVHNode() = default;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!box.hit(r, ray_t)) {
            return false;
        }

        if (is_leaf) {
            bool hit_anything = false;
            interval closest_so_far = ray_t;

            // Check all objects in leaf node
            for (const auto& object : leaf_objects) {
                if (object->hit(r, closest_so_far, rec)) {
                    hit_anything = true;
                    closest_so_far.max = rec.t;
                }
            }

            return hit_anything;
        }

        // Internal node traversal
        bool hit_left = left && left->hit(r, ray_t, rec);
        interval right_t(ray_t.min, hit_left ? rec.t : ray_t.max);
        bool hit_right = right && right->hit(r, right_t, rec);

        return hit_left || hit_right;
    }

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        // Early exit if the ray doesn't intersect the bounding box
        if (!box.hit(r, ray_t)) {
            return false;
        }

        bool hit_anything = false;

        if (is_leaf) {
            // Leaf node: check all objects in the leaf
            for (const auto& object : leaf_objects) {
                std::vector<hit_record> temp_recs;
                if (object->hit_all(r, ray_t, temp_recs)) {
                    hit_anything = true;
                    for (const auto& rec : temp_recs) {
                        // Only add records within the ray interval range
                        if (rec.t >= ray_t.min && rec.t <= ray_t.max) {
                            recs.push_back(rec);
                            ray_t.max = rec.t;  // Update the interval to limit further searches
                        }
                    }
                }
            }
        }
        else {
            // Internal node: traverse left and right children with interval updates
            std::vector<hit_record> left_recs, right_recs;

            if (left && left->hit_all(r, ray_t, left_recs)) {
                hit_anything = true;
                for (const auto& rec : left_recs) {
                    if (rec.t >= ray_t.min && rec.t <= ray_t.max) {
                        recs.push_back(rec);
                        ray_t.max = rec.t;  // Update the ray interval to prioritize closer hits
                    }
                }
            }

            if (right && right->hit_all(r, ray_t, right_recs)) {
                hit_anything = true;
                for (const auto& rec : right_recs) {
                    if (rec.t >= ray_t.min && rec.t <= ray_t.max) {
                        recs.push_back(rec);
                        ray_t.max = rec.t;  // Further limit the search interval
                    }
                }
            }
        }

        // Sort all hits by t value if there were any hits
        if (hit_anything) {
            std::sort(recs.begin(), recs.end(), [](const hit_record& a, const hit_record& b) {
                return a.t < b.t;
                });
        }

        return hit_anything;
    }

    BoundingBox bounding_box() const override {
        return box;
    }

    bool is_point_inside(const point3& p) const override {
        // Check if the point is inside the BVH node's bounding box first (early exit)
        if (!box.contains(p)) {
            return false;
        }

        if (is_leaf) {
            // If this is a leaf node, check all objects in the leaf
            for (const auto& object : leaf_objects) {
                if (object->is_point_inside(p)) {
                    return true;
                }
            }
            return false;
        }
        else {
            // If it's an internal node, recursively check left and right children
            bool insideLeft = left && left->is_point_inside(p);
            bool insideRight = right && right->is_point_inside(p);

            return insideLeft || insideRight;
        }
    }

};

#endif // BVH_NODE_H