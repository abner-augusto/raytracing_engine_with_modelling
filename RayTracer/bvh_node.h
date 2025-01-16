#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "boundingbox.h"
#include "hittable.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

class BVHNode : public hittable {
private:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    BoundingBox box;

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

        if (end - start <= 2) {
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

        node->box = node->left->bounding_box().enclose(node->right->bounding_box());
        return node;
    }

    void buildLeafNode(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
        if (end - start == 1) {
            left = right = objects[start];
            box = objects[start]->bounding_box();
        }
        else {  // end - start == 2
            left = objects[start];
            right = objects[start + 1];
            box = left->bounding_box().enclose(right->bounding_box());
        }
    }

public:
    BVHNode() = default;

    BVHNode(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
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
            }
        }
    }

    virtual ~BVHNode() = default;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!box.hit(r, ray_t, rec)) {
            return false;
        }

        bool hit_left = left && left->hit(r, ray_t, rec);
        interval right_t(ray_t.min, hit_left ? rec.t : ray_t.max);
        bool hit_right = right && right->hit(r, right_t, rec);

        return hit_left || hit_right;
    }

    BoundingBox bounding_box() const override {
        if (!left && !right) {
            throw std::runtime_error("BVHNode does not have children to compute a bounding box.");
        }
        return box;
    }
};

#endif // BVH_NODE_H
