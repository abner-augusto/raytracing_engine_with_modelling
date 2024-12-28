#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <string>
#include <stdexcept>
#include "node.h"
#include "boundingBox.h"
#include "sphere.h"
#include "vec3.h"

class Octree {
public:
    BoundingBox bounding_box;
    Node root;

    Octree(const BoundingBox& bb, const Node& r) : bounding_box(bb), root(r) {}

    static Octree FromObject(const BoundingBox& bb, const sphere& obj, int depth_limit = 3) {
        Node root = Node::FromObject(bb, obj, depth_limit);
        return Octree(bb, root);
    }

    static Octree FromString(const BoundingBox& bb, const std::string& input) {
        size_t pos = 0;
        Node root = Node::FromStringRecursive(input, pos);
        return Octree(bb, root);
    }

    static Octree FromBooleanOperation(const Octree& octree1, const Octree& octree2,
        const BoundingBox& result_bb, const std::string& operation) {
        // Start with an empty root node
        Node new_root = Node::EmptyNode();

        // Perform the recursive Boolean operation
        new_root = Node::BooleanRecursive(octree1.root, octree1.bounding_box,
            octree2.root, octree2.bounding_box,
            result_bb, operation);

        // Return the resulting octree
        return Octree(result_bb, new_root);
    }

    std::vector<BoundingBox> GetFilledBoundingBoxes() const {
        return root.GetFilledBoundingBoxes(bounding_box);
    }

    bool TestPoint(const point3& p) const {
        return root.TestPoint(bounding_box, p);
    }

    std::string ToString() const {
        return root.ToString();
    }
};

#endif // OCTREE_H
