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

    static Octree FromBooleanOperation(const Octree& octree1,
        const Octree& octree2,
        const std::string& operation,
        int maxDepth)
    {
        // Check if the bounding boxes are not aligned
        if (octree1.bounding_box != octree2.bounding_box) {
            BoundingBox mergedBB = octree1.bounding_box.Enclose(octree2.bounding_box);

            // Rebuild each old octree to use the merged bounding box
            Octree rebuilt1 = RebuildOctreeFromBbs(octree1, mergedBB, maxDepth);
            Octree rebuilt2 = RebuildOctreeFromBbs(octree2, mergedBB, maxDepth);

            // Perform the boolean operation using the merged bounding box
            Node newRoot = Node::BooleanRecursive(
                rebuilt1.root,
                rebuilt2.root,
                operation
            );

            return Octree(mergedBB, newRoot);
        }

        // Bounding boxes are aligned, perform the boolean operation directly
        Node newRoot = Node::BooleanRecursive(
            octree1.root,
            octree2.root,
            operation
        );
        return Octree(octree1.bounding_box, newRoot);
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

    static Octree RebuildOctreeFromBbs(const Octree& oldOctree,
        const BoundingBox& newBB,
        int maxDepth)
    {
        // 1) Extract the “filled” bounding boxes from the old octree
        std::vector<BoundingBox> oldFilledBbs =
            oldOctree.root.GetFilledBoundingBoxes(oldOctree.bounding_box);

        // 2) Build the new root node by subdividing based on these bounding boxes
        Node newRoot = Node::RebuildFromFilledBbs(
            oldFilledBbs,
            newBB,
            maxDepth,
            0
        );

        return Octree(newBB, newRoot);
    }
};

#endif // OCTREE_H
