#ifndef CSG_H
#define CSG_H

#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include <cmath>
#include "hittable.h"
#include "boundingbox.h"

class CSGHitList {
public:
    std::vector<CSGIntersection> intersections;

    void add_intersection(double t, bool is_entry, const hittable* obj,
        const vec3& normal, const point3& p) {
        intersections.emplace_back(t, is_entry, obj, normal, p);
    }

    size_t size() const { return intersections.size(); }
    void clear() { intersections.clear(); }

    void sort() {
        std::sort(intersections.begin(), intersections.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });
    }
};

// A wrapper that stores a single geometry object as a CSG "primitive".
class CSGPrimitive : public hittable {
public:
    explicit CSGPrimitive(std::shared_ptr<hittable> object)
        : object(std::move(object)), has_cached_bb(false) {
    }

    // Normal raytracing hit
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Retrieve the bounding box of the wrapped object
        BoundingBox bbox = this->bounding_box();

        // Early exit if the ray does not intersect the bounding box
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        // Delegate to the wrapped object's hit method
        return object->hit(r, ray_t, rec);
    }

    // Collect all intersections for CSG logic
    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override {
        // Delegate to child
        return object->csg_intersect(r, ray_t, out_intersections);
    }

    bool is_point_inside(const point3& p) const override {
        return object->is_point_inside(p);
    }

    char test_bb(const BoundingBox& bb) const override {
        return object->test_bb(bb);
    }

    BoundingBox bounding_box() const override {
        //std::lock_guard<std::mutex> lock(bb_mutex);
        if (!has_cached_bb) {
            cached_bb = object->bounding_box();
            has_cached_bb = true;
        }
        return cached_bb;
    }

    void transform(const Matrix4x4& matrix) override {
        //std::lock_guard<std::mutex> lock(bb_mutex);
        object->transform(matrix);
        has_cached_bb = false;
    }

    std::string get_type_name() const override {
        return "CSGPrimitive<" + object->get_type_name() + ">";
    }

    mat get_material() const override {
        return object->get_material();
    }

    void set_material(const mat& new_material) override {
        object->set_material(new_material);
    }

private:
    std::shared_ptr<hittable> object;
    // Mutable members for lazy BB calculation
    mutable bool has_cached_bb;
    mutable BoundingBox cached_bb;

    // Optional: Mutex for thread safety
    // mutable std::mutex bb_mutex;
};

enum class CSGType { NONE, UNION, INTERSECTION, DIFFERENCE };

inline std::string csg_type_to_string(CSGType type) {
    switch (type) {
    case CSGType::UNION: return "Union";
    case CSGType::INTERSECTION: return "Intersection";
    case CSGType::DIFFERENCE: return "Difference";
    default: return "Unknown";
    }
}

/*
 The CSGNode uses an Operation (Union, Intersection, Difference).
 For rendering: "hit()" picks the closest intersection that is actually
 in the resulting boolean volume. For CSG logic: "csg_intersect()" merges
 the sub-intersections from children.
 */
template <typename Operation>
class CSGNode : public hittable {
public:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    BoundingBox bbox;

    CSGNode(std::shared_ptr<hittable> leftChild,
        std::shared_ptr<hittable> rightChild)
        : left(std::move(leftChild)), right(std::move(rightChild))
    {
        // Combine bounding boxes according to the Boolean op
        bbox = Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        // Early bounding box test
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        // Gather CSG intersections
        std::vector<CSGIntersection> csgHits;
        bool has_intersections = csg_intersect(r, ray_t, csgHits);

        if (!has_intersections || csgHits.empty()) {
            return false;
        }

        // Get first hit (should be closest valid hit)
        const auto& firstHit = csgHits[0];


        if (firstHit.t < ray_t.min || firstHit.t > ray_t.max) {
            return false;
        }

        // Create the hit record
        rec.t = firstHit.t;
        rec.p = firstHit.p;
        rec.normal = firstHit.normal;
        rec.front_face = firstHit.is_entry;
        rec.hit_object = this;

        // Get material from the original primitive
        hit_record temp_rec;
        bool got_material = firstHit.obj->hit(r, interval(firstHit.t - 0.0001, firstHit.t + 0.0001), temp_rec);

        if (got_material) {
            rec.material = temp_rec.material;
            rec.u = temp_rec.u;
            rec.v = temp_rec.v;
            return true;
        }

        return false;
    }

    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override
    {
        out_intersections.clear();

        // Early exit if the bounding box is missed.
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        // Gather intersections from child nodes.
        std::vector<CSGIntersection> leftHits;
        std::vector<CSGIntersection> rightHits;

        if (left->bounding_box().hit(r, ray_t)) {
            left->csg_intersect(r, ray_t, leftHits);
        }
        if (right->bounding_box().hit(r, ray_t)) {
            right->csg_intersect(r, ray_t, rightHits);
        }


    // --- Early exit conditions ---
    if (Operation::csg_type == CSGType::UNION) {
        // For a union, if one side is empty, the result is just the other side.
        if (leftHits.empty() && rightHits.empty()) {
            return false;
        }
        if (leftHits.empty()) {
            out_intersections = rightHits;
            std::sort(out_intersections.begin(), out_intersections.end(),
                [](const CSGIntersection& a, const CSGIntersection& b) {
                    return a.t < b.t;
                });
            return true;
        }
        if (rightHits.empty()) {
            out_intersections = leftHits;
            std::sort(out_intersections.begin(), out_intersections.end(),
                [](const CSGIntersection& a, const CSGIntersection& b) {
                    return a.t < b.t;
                });
            return true;
        }
    }

    // ------------------------------------------------------
        // Merge the events from both children.
        std::vector<CSGIntersection> events;
        events.reserve(leftHits.size() + rightHits.size());

        for (auto& hit : leftHits) {
            hit.obj = left.get();  // Ensure the object pointer is set correctly.
            events.push_back(hit);
        }
        for (auto& hit : rightHits) {
            hit.obj = right.get();  // Ensure the object pointer is set correctly.
            events.push_back(hit);
        }

        // Sort all events by ascending t.
        std::sort(events.begin(), events.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });

        if (events.empty()) {
            return false;
        }

        // Determine the initial inside/outside states.
        double eps = 1e-12;
        double start_t = std::max(ray_t.min, 0.0) + eps;
        point3 start_point = r.at(start_t);
        bool insideLeft = left->is_point_inside(start_point);
        bool insideRight = right->is_point_inside(start_point);
        bool was_in_csg = Operation::in_csg(insideLeft, insideRight);

        // Traverse events, toggling states and recording transitions.
        for (auto& ev : events) {
            if (ev.t < ray_t.min || ev.t > ray_t.max) {
                continue;
            }
            // Toggle state based on which child the event belongs to.
            if (ev.obj == left.get()) {
                insideLeft = ev.is_entry;
            }
            else {
                insideRight = ev.is_entry;
            }
            bool now_in_csg = Operation::in_csg(insideLeft, insideRight);
            if (now_in_csg != was_in_csg) {
                auto& final_hit = ev;
                final_hit.is_entry = now_in_csg;
                final_hit.normal = ev.normal;
                out_intersections.push_back(final_hit);
            }
            was_in_csg = now_in_csg;
        }

        std::sort(out_intersections.begin(), out_intersections.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });

        return !out_intersections.empty();
    }



    bool is_point_inside(const point3& p) const override {
        bool inLeft = left->is_point_inside(p);
        bool inRight = right->is_point_inside(p);
        return Operation::in_csg(inLeft, inRight);
    }

    void transform(const Matrix4x4& matrix) override {
        // Propagate the transformation to both child nodes.
        left->transform(matrix);
        right->transform(matrix);

        // Recompute the bounding box after transformation.
        bbox = Operation::bounding_box(left->bounding_box(), right->bounding_box());
    }

    BoundingBox bounding_box() const override {
        return bbox;
    }

    std::string get_type_name() const override {
        return "CSGNode<" + csg_type_to_string(Operation::csg_type) + ">";
    }

    char test_bb(const BoundingBox& bb) const override {
        // First check if the test box intersects our overall bounding box
        if (!bbox.intersects(bb)) {
            return 'w';  // completely outside
        }

        const auto vertices = bb.getVertices();

        // Count how many corners are inside the CSG result
        unsigned int inside_count = 0;
        for (const auto& v : vertices) {
            if (is_point_inside(v)) {
                inside_count++;
            }
        }

        if (inside_count == 8) {
            // For unions and intersections, we need to verify with both children
            // For difference, we only need to verify with left child
            if (Operation::csg_type == CSGType::DIFFERENCE) {
                return left->test_bb(bb);
            }
            else {
                // Get classification from both children
                char left_result = left->test_bb(bb);
                char right_result = right->test_bb(bb);

                if (Operation::csg_type == CSGType::UNION) {
                    // For union, if either child fully contains the box, the result fully contains it
                    if (left_result == 'b' || right_result == 'b') return 'b';
                    return 'g';  // Otherwise it must be partial
                }
                else if (Operation::csg_type == CSGType::INTERSECTION) {
                    // For intersection, both children must fully contain the box
                    if (left_result == 'b' && right_result == 'b') return 'b';
                    return 'g';  // Otherwise it must be partial
                }
            }
        }
        else if (inside_count == 0) {
            // Check the center and face centers for potential intersections
            point3 c = bb.getCenter();
            if (is_point_inside(c)) {
                return 'g';
            }

            // Check face centers
            point3 dims = bb.getDimensions();
            vec3 half_dims = dims / 2.0;
            point3 faceCenters[6] = {
                bb.vmin + vec3(half_dims.x(), half_dims.y(), 0),                 // front
                bb.vmin + vec3(half_dims.x(), half_dims.y(), dims.z()),          // back
                bb.vmin + vec3(half_dims.x(), 0, half_dims.z()),                 // bottom
                bb.vmin + vec3(half_dims.x(), dims.y(), half_dims.z()),          // top
                bb.vmin + vec3(0, half_dims.y(), half_dims.z()),                 // left
                bb.vmin + vec3(dims.x(), half_dims.y(), half_dims.z())           // right
            };

            for (const auto& fc : faceCenters) {
                if (is_point_inside(fc)) {
                    return 'g';
                }
            }

            // No corners inside and no centers inside
            // For union, check if either child intersects
            if (Operation::csg_type == CSGType::UNION) {
                char left_result = left->test_bb(bb);
                char right_result = right->test_bb(bb);
                if (left_result == 'w' && right_result == 'w') return 'w';
                return 'g';
            }

            return 'w';
        }

        // Some corners in, some out => partial
        return 'g';
    }

};

// Union operation
struct Union {
    static bool in_csg(bool inLeft, bool inRight) {
        return (inLeft || inRight);
    }

    static constexpr CSGType csg_type = CSGType::UNION;


    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox.enclose(rightBox);
    }

    static std::string get_type_name() { return "CSGNode(Union)"; }
};


// Intersection operation
struct Intersection {
    static constexpr CSGType csg_type = CSGType::INTERSECTION;

    static bool in_csg(bool in_left, bool in_right) {
        return in_left && in_right;
    }


    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        if (!leftBox.intersects(rightBox)) return BoundingBox();
        return leftBox.from_intersect(rightBox);
    }

    static std::string get_type_name() { return "CSGNode(Intersection)"; }
};

// Difference operation
struct Difference {
    static constexpr CSGType csg_type = CSGType::DIFFERENCE;

    static bool in_csg(bool in_left, bool in_right) {
        return in_left && !in_right;
    }

    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox;
    }

    static std::string get_type_name() { return "CSGNode(Difference)"; }
};

inline void print_csg_tree(const std::shared_ptr<hittable>& node, int depth = 0, const std::string& prefix = "", bool is_last_child = true) {
    const std::string branch = "L__ ";
    const std::string vertical = "|   ";
    const std::string last_branch = "\\__ ";
    const std::string space = "    ";

    // Print the prefix
    std::cout << prefix;

    // Print the branch symbol
    if (depth > 0) {
        std::cout << (is_last_child ? last_branch : branch);
    }

    // Check if the node is a CSGPrimitive
    if (auto primitive = std::dynamic_pointer_cast<CSGPrimitive>(node)) {
        std::cout << "CSGPrimitive(" << primitive->get_type_name() << ")" << std::endl;
    }
    // Check if the node is a CSGNode
    else if (auto csg_node = std::dynamic_pointer_cast<CSGNode<Union>>(node)) {
        std::cout << "CSGNode(Union)" << std::endl;
        print_csg_tree(csg_node->left, depth + 1, prefix + (is_last_child ? space : vertical), false);
        print_csg_tree(csg_node->right, depth + 1, prefix + (is_last_child ? space : vertical), true);
    }
    else if (auto csg_node = std::dynamic_pointer_cast<CSGNode<Intersection>>(node)) {
        std::cout << "CSGNode(Intersection)" << std::endl;
        print_csg_tree(csg_node->left, depth + 1, prefix + (is_last_child ? space : vertical), false);
        print_csg_tree(csg_node->right, depth + 1, prefix + (is_last_child ? space : vertical), true);
    }
    else if (auto csg_node = std::dynamic_pointer_cast<CSGNode<Difference>>(node)) {
        std::cout << "CSGNode(Difference)" << std::endl;
        print_csg_tree(csg_node->left, depth + 1, prefix + (is_last_child ? space : vertical), false);
        print_csg_tree(csg_node->right, depth + 1, prefix + (is_last_child ? space : vertical), true);
    }
    else {
        std::cout << "Unknown CSG Node Type" << std::endl;
    }
}

// Helper function to recursively gather CSG intersections with visited tracking
inline  void gather_intersections_recursive(
    const hittable* obj,
    const ray& r,
    const interval& interval,
    std::vector<CSGIntersection>& intersections,
    std::unordered_set<const hittable*>& visited
) {
    // Check if the node has already been processed
    if (visited.find(obj) != visited.end()) {
        return; // Already processed, skip to prevent duplicates
    }

    // Mark the current node as visited
    visited.insert(obj);

    std::vector<CSGIntersection> local_intersections;
    if (obj->csg_intersect(r, interval, local_intersections)) {
        for (const auto& intersect : local_intersections) {
            intersections.push_back(intersect);
            // Check if the intersected object is a CSG node
            bool child_is_csg_node = dynamic_cast<const CSGNode<Union>*>(intersect.obj) ||
                dynamic_cast<const CSGNode<Intersection>*>(intersect.obj) ||
                dynamic_cast<const CSGNode<Difference>*>(intersect.obj);
            if (child_is_csg_node) {
                gather_intersections_recursive(intersect.obj, r, interval, intersections, visited);
            }
        }
    }
}

inline  void log_csg_hits(HittableManager& manager, const ray& central_ray) {
    interval ray_interval(0.001, infinity);
    hit_record closest_hit;

    // First, find the closest hit using the regular hit() method
    if (manager.hit(central_ray, ray_interval, closest_hit)) {
        std::cout << "\n=== Closest Hit Along Central Ray ===\n";
        std::cout << "  t Value: " << closest_hit.t << "\n";
        std::cout << "  Position: ("
            << closest_hit.p.x() << ", "
            << closest_hit.p.y() << ", "
            << closest_hit.p.z() << ")\n";
        std::cout << "  Normal: ("
            << closest_hit.normal.x() << ", "
            << closest_hit.normal.y() << ", "
            << closest_hit.normal.z() << ")\n";
        std::cout << "  Front Face: " << (closest_hit.front_face ? "Yes" : "No") << "\n";
        std::cout << "  Object Type: " << closest_hit.hit_object->get_type_name() << "\n";
        std::cout << "  Object Pointer: " << closest_hit.hit_object << "\n";

        // Check if the closest hit is a CSG node
        bool is_csg_node = dynamic_cast<const CSGNode<Union>*>(closest_hit.hit_object) ||
            dynamic_cast<const CSGNode<Intersection>*>(closest_hit.hit_object) ||
            dynamic_cast<const CSGNode<Difference>*>(closest_hit.hit_object);

        if (is_csg_node) {
            // Directly proceed to handle the CSG node without redundant logging
            std::cout << "\n  CSG Node Tree:\n";
            std::cout << "\n";
            // Use a non-owning shared_ptr with a no-op deleter
            std::shared_ptr<hittable> non_owning_ptr(
                const_cast<hittable*>(closest_hit.hit_object),
                [](hittable*) { /* No-op deleter */ }
            );
            print_csg_tree(non_owning_ptr);

            std::cout << "\n";

            // Collect all CSG intersections, including recursive ones
            std::vector<CSGIntersection> all_csg_intersections;
            std::unordered_set<const hittable*> visited_nodes;

            // Start gathering from the closest CSG node
            gather_intersections_recursive(closest_hit.hit_object, central_ray, ray_interval, all_csg_intersections, visited_nodes);

            if (!all_csg_intersections.empty()) {
                // Sort the intersections by t to group duplicates
                std::sort(all_csg_intersections.begin(), all_csg_intersections.end(),
                    [](const CSGIntersection& a, const CSGIntersection& b) {
                        return a.t < b.t;
                    });

                // Define an epsilon for floating-point comparison
                const double epsilon = 1e-8;

                // Use std::unique to remove consecutive duplicates based on t, obj, and is_entry
                auto new_end = std::unique(all_csg_intersections.begin(), all_csg_intersections.end(),
                    [epsilon](const CSGIntersection& a, const CSGIntersection& b) {
                        return std::abs(a.t - b.t) < epsilon &&
                            a.obj == b.obj &&
                            a.is_entry == b.is_entry;
                    });

                // Erase the leftover elements
                all_csg_intersections.erase(new_end, all_csg_intersections.end());

                // Log each unique intersection
                for (size_t i = 0; i < all_csg_intersections.size(); i++) {
                    const auto& intersection = all_csg_intersections[i];

                    std::cout << "\n---------------------------------\n";
                    std::cout << "CSG Intersection #" << (i + 1) << ":\n";
                    std::cout << "  t Value: " << intersection.t << "\n";

                    std::cout << "  Position: ("
                            << intersection.p.x() << ", "
                            << intersection.p.y() << ", "
                            << intersection.p.z() << ")\n";
                    std::cout << "  Normal: ("
                            << intersection.normal.x() << ", "
                            << intersection.normal.y() << ", "
                            << intersection.normal.z() << ")\n";
                    

                    std::cout << "  Is Entry: " << (intersection.is_entry ? "Yes" : "No") << "\n";
                    if (intersection.obj) {
                        std::cout << "  Object Type: " << intersection.obj->get_type_name() << "\n";
                    }
                    else {
                        std::cout << "  Object Type: NULL\n";
                    }
                    std::cout << "  Object Pointer: " << intersection.obj << "\n";

                    // If the intersected object is a CSG node, print its tree
                    bool is_child_csg_node = dynamic_cast<const CSGNode<Union>*>(intersection.obj) ||
                        dynamic_cast<const CSGNode<Intersection>*>(intersection.obj) ||
                        dynamic_cast<const CSGNode<Difference>*>(intersection.obj);

                    if (is_child_csg_node) {
                        std::cout << "  CSG Node Tree:\n";
                        std::cout << "\n";
                        std::shared_ptr<hittable> child_non_owning_ptr(
                            const_cast<hittable*>(intersection.obj),
                            [](hittable*) { /* No-op deleter */ }
                        );
                        print_csg_tree(child_non_owning_ptr);
                        std::cout << "\n";
                    }
                }

                // SMC Log
                std::cout << "\n============ SMC of Ray Traversal ============\n";
                std::cout << std::left << std::setw(10) << "t"
                    << std::left << std::setw(25) << "Object"
                    << "Status\n";
                std::cout << "----------------------------------------------\n";

                // Initialize state
                bool inside = false;
                for (const auto& intersect : all_csg_intersections) {
                    // Get object type/name
                    std::string object_name = intersect.obj->get_type_name();
                    std::string status = intersect.is_entry ? "In" : "Out";

                    // Use fixed-width fields for alignment
                    std::cout << std::fixed << std::setprecision(5);
                    std::cout << std::left << std::setw(10) << intersect.t
                        << std::left << std::setw(25) << object_name
                        << std::left << std::setw(5) << status << "\n";
                }

                std::cout << "----------------------------------------------\n";
            }
            else {
                std::cout << "No CSG intersections found for the closest CSG node.\n";
            }
        }
        else {
            std::cout << "\nThe closest hit is not a CSG node.\n";
        }
    }
    else {
        std::cout << "No hits along the central ray.\n";
    }
}

#endif // CSG_H