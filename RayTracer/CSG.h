#ifndef CSG_H
#define CSG_H

#include <memory>
#include "hittable.h"
#include "boundingbox.h"


class CSGHitList {
public:
    std::vector<CSGIntersection> intersections;

    // Add intersection with correct argument types, including is_entry
    void add_intersection(double t, bool is_entry, const hittable* obj,
        const vec3& normal, const point3& p,
        const mat* material) {
        intersections.emplace_back(t, is_entry, obj, normal, p, material);
    }

    size_t size() const { return intersections.size(); }
    void clear() { intersections.clear(); }

    void sort() {
        std::sort(intersections.begin(), intersections.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;  // Sort by intersection time
            });
    }
};

class CSGPrimitive : public hittable {
public:
    explicit CSGPrimitive(std::shared_ptr<hittable> object)
        : object(std::move(object)) {
    }

    // For a single closest hit, we can rely on hit_all(...) and pick the first
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        std::vector<hit_record> recs;
        if (!hit_all(r, ray_t, recs) || recs.empty()) {
            return false;
        }
        // Among all hits, pick the earliest valid
        double closest_t = ray_t.max;
        bool found_any = false;
        for (auto& h : recs) {
            if (h.t < closest_t && h.t >= ray_t.min) {
                closest_t = h.t;
                rec = h;
                found_any = true;
            }
        }
        return found_any;
    }

    // Return all intersections with the wrapped object
    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        if (!object->hit_all(r, ray_t, recs)) {
            return false;
        }
        for (auto& hr : recs) {
            hr.hit_object = this;  // Set hit_object to the CSGPrimitive
            hr.csg_intersections.clear(); // Remove any existing CSG entries
        }
        return true;
    }

    bool is_point_inside(const point3& p) const override {
        // Delegates to the child geometry’s is_point_inside
        return object->is_point_inside(p);
    }

    BoundingBox bounding_box() const override {
        return object->bounding_box();
    }

    std::string get_type_name() const override {
        return object->get_type_name();
    }

private:
    std::shared_ptr<hittable> object;
};

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
        // Collect all boundary crossings, pick the earliest that enters the CSG shape
        std::vector<hit_record> recs;
        if (!hit_all(r, ray_t, recs) || recs.empty()) {
            return false;
        }
        // Among all boundary events, find the first one where we transition from OUT -> IN
        double closest_t = ray_t.max;
        bool found_hit = false;
        for (auto& hr : recs) {
            // If hr.front_face == true => we are entering the shape
            if (hr.front_face && hr.t < closest_t && hr.t >= ray_t.min) {
                closest_t = hr.t;
                rec = hr;
                found_hit = true;
            }
        }
        return found_hit;
    }

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        recs.clear();

        // Quick bounding-box test
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        // 1) Collect ALL raw intersections from both children
        std::vector<hit_record> leftRecords, rightRecords;
        left->hit_all(r, ray_t, leftRecords);
        right->hit_all(r, ray_t, rightRecords);

        // 2) Build a merged list of CSGIntersection
        std::vector<CSGIntersection> events;
        events.reserve(leftRecords.size() + rightRecords.size());

        auto add_to_events = [&](const std::vector<hit_record>& childRecs, bool isLeftChild) {
            for (const auto& hr : childRecs) {
                bool is_entry = hr.front_face;  // Determine entry based on front_face value

                events.emplace_back(
                    hr.t,               // Intersection time
                    is_entry,            // Entry or exit flag
                    hr.hit_object,      // Pointer to intersected object
                    hr.normal,          // Surface normal
                    hr.p,               // Intersection point
                    hr.material         // Material pointer
                );
            }
            };

        add_to_events(leftRecords, true);
        add_to_events(rightRecords, false);

        // Sort by ascending t value
        std::sort(events.begin(), events.end(), [](const CSGIntersection& a, const CSGIntersection& b) {
            return a.t < b.t;
            });

        if (events.empty()) {
            return false;
        }

        // 3) Determine initial inside/outside state for left & right
        double eps = 1e-6;
        double start_t = std::max(ray_t.min, 0.0) + eps;
        point3 start_point = r.at(start_t);

        bool insideLeft = left->is_point_inside(start_point);
        bool insideRight = right->is_point_inside(start_point);

        // Combined shape state
        bool was_in_csg = Operation::in_csg(insideLeft, insideRight);

        // 4) Traverse events in ascending t, toggling inside states
        for (const auto& ev : events) {
            if (ev.t < ray_t.min || ev.t > ray_t.max) {
                continue; // Skip out-of-range intersections
            }

            // Determine which object was hit
            bool isLeft = (ev.obj == left.get());
            bool isRight = (ev.obj == right.get());

            if (!isLeft && !isRight) {
                continue;  // Shouldn't happen, avoid undefined behavior
            }

            // Toggle that shape’s inside/outside state
            if (isLeft) insideLeft = !insideLeft;
            if (isRight) insideRight = !insideRight;

            bool now_in_csg = Operation::in_csg(insideLeft, insideRight);

            // If there's a transition from out->in or in->out, record the intersection
            if (now_in_csg != was_in_csg) {
                hit_record hr;
                hr.t = ev.t;
                hr.p = ev.p;
                hr.material = ev.material;
                hr.hit_object = this;
                hr.csg_op = Operation::csg_type;

                // Determine if entering or exiting the CSG volume
                hr.front_face = now_in_csg;

                // Adjust normal to always point outward relative to the CSG surface
                if (now_in_csg) {
                    // Entering CSG: use primitive's outward normal (entry) or flipped exit normal
                    hr.normal = ev.normal;
                }
                else {
                    // Exiting CSG: flip the primitive's normal to point outward from CSG
                    hr.normal = -ev.normal;
                }

                recs.push_back(hr);
            }

            was_in_csg = now_in_csg;
        }

        return !recs.empty();
    }

    bool is_point_inside(const point3& p) const override {
        bool inLeft = left->is_point_inside(p);
        bool inRight = right->is_point_inside(p);
        return Operation::in_csg(inLeft, inRight);
    }

    BoundingBox bounding_box() const override {
        return bbox;
    }

    std::string get_type_name() const override {
        return "CSGNode<" + std::to_string(static_cast<int>(Operation::csg_type)) + ">";
    }

};

// Union operation
struct Union {
    static bool in_csg(bool inLeft, bool inRight) {
        return (inLeft || inRight);
    }

    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::UNION;


    static BoundingBox bounding_box(const BoundingBox& leftBox, const BoundingBox& rightBox) {
        return leftBox.enclose(rightBox);
    }

    static std::string get_type_name() { return "CSGNode(Union)"; }
};


// Intersection operation
struct Intersection {
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::INTERSECTION;

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
    static constexpr hit_record::CSGType csg_type = hit_record::CSGType::DIFFERENCE;

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

void log_csg_children(const hittable* node, const std::string& prefix, bool is_left = false) {
    if (!node) {
        std::cout << prefix << "nullptr\n";
        return;
    }

    std::string child_prefix = prefix + "  "; // Indentation for child nodes

    // Determine and print the appropriate node type
    if (auto primitive = dynamic_cast<const CSGPrimitive*>(node)) {
        std::cout << prefix << (is_left ? "Left:  " : "Right: ")
            << "CSGPrimitive(" << primitive->get_type_name() << ")\n";
    }
    else if (auto csg_union = dynamic_cast<const CSGNode<Union>*>(node)) {
        std::cout << prefix << (is_left ? "Left:  " : "Right: ") << "CSGNode(Union)\n";
        log_csg_children(csg_union->left.get(), child_prefix, true);
        log_csg_children(csg_union->right.get(), child_prefix, false);
    }
    else if (auto csg_intersection = dynamic_cast<const CSGNode<Intersection>*>(node)) {
        std::cout << prefix << (is_left ? "Left:  " : "Right: ") << "CSGNode(Intersection)\n";
        log_csg_children(csg_intersection->left.get(), child_prefix, true);
        log_csg_children(csg_intersection->right.get(), child_prefix, false);
    }
    else if (auto csg_difference = dynamic_cast<const CSGNode<Difference>*>(node)) {
        std::cout << prefix << (is_left ? "Left:  " : "Right: ") << "CSGNode(Difference)\n";
        log_csg_children(csg_difference->left.get(), child_prefix, true);
        log_csg_children(csg_difference->right.get(), child_prefix, false);
    }
    else {
        std::cout << prefix << "Unknown CSG Node Type\n";
    }
}

void log_csg_hits(HittableManager& manager, const ray& central_ray) {
    // Collect all hit records along the ray
    std::vector<hit_record> hits;
    interval ray_interval(0.001, infinity);

    if (manager.hit_all(central_ray, ray_interval, hits)) {
        std::cout << "\n=== CSG Hits Along Central Ray ===\n";

        for (size_t i = 0; i < hits.size(); i++) {
            const auto& rec = hits[i];

            std::cout << "\n---------------------------------\n";
            std::cout << "Hit #" << (i + 1) << ":\n";
            std::cout << "  t Value: " << rec.t << "\n";
            std::cout << "  Position: ("
                << rec.p.x() << ", "
                << rec.p.y() << ", "
                << rec.p.z() << ")\n";

            std::cout << "  Normal: ("
                << rec.normal.x() << ", "
                << rec.normal.y() << ", "
                << rec.normal.z() << ")\n";

            std::cout << "  Front Face: " << (rec.front_face ? "Yes" : "No") << "\n";

            // Check if the hit object is a CSG node
            bool is_csg_node = dynamic_cast<const CSGNode<Union>*>(rec.hit_object) ||
                dynamic_cast<const CSGNode<Intersection>*>(rec.hit_object) ||
                dynamic_cast<const CSGNode<Difference>*>(rec.hit_object);

            if (is_csg_node) {
                std::cout << "  Event: CSG Hit\n"; // Explicitly mark as CSG hit
                if (!rec.csg_intersections.empty()) {
                    std::cout << "  CSG Intersections Count: " << rec.csg_intersections.size() << "\n";
                    for (size_t j = 0; j < rec.csg_intersections.size(); j++) {
                        const auto& intersection = rec.csg_intersections[j];
                        std::cout << "    Intersection " << (j + 1) << ":\n";
                        std::cout << "      t Value: " << intersection.t << "\n";
                        std::cout << "      Position: ("
                            << intersection.p.x() << ", "
                            << intersection.p.y() << ", "
                            << intersection.p.z() << ")\n";

                        std::cout << "      Normal: ("
                            << intersection.normal.x() << ", "
                            << intersection.normal.y() << ", "
                            << intersection.normal.z() << ")\n";

                        if (intersection.obj) {
                            std::cout << "      Object Type: " << intersection.obj->get_type_name() << "\n";
                        }
                        else {
                            std::cout << "      Object Type: ERROR (nullptr)\n";
                        }
                    }
                }
                else {
                    std::cout << "  CSG Intersections: None (but hit object is a CSG node)\n";
                }
            }
            else {
                std::cout << "  Event: Not a CSG hit\n";
            }

            // Object information
            std::cout << "  Object Type: ";
            if (rec.hit_object) {
                std::cout << rec.hit_object->get_type_name();
            }
            else {
                std::cout << "ERROR: hit_object is nullptr";
            }

            std::cout << "\n  Object Pointer: " << rec.hit_object << "\n";

            // If the hit object is a CSG node, log its children
            if (rec.hit_object && is_csg_node) {
                std::cout << "  CSG Node Children:\n";
                log_csg_children(rec.hit_object, "    ");
            }
        }

        std::cout << "---------------------------------\n\n";
    }
    else {
        std::cout << "No hits along the central ray.\n";
    }
}

#endif // CSG_H