#ifndef NODE_H
#define NODE_H

#include <vector>
#include <cassert>
#include "boundingbox.h"

class OctreeNode {
public:
    bool is_filled;
    std::vector<OctreeNode> children; // size 0 or 8

    // Default constructor
    OctreeNode() : is_filled(false), children() {}

    OctreeNode(bool filled, const std::vector<OctreeNode>& ch = {})
        : is_filled(filled), children(ch) {
    }

    static OctreeNode EmptyNode() {
        return OctreeNode(false, {});
    }

    static OctreeNode FullNode() {
        return OctreeNode(true, {});
    }

    // Constructs an octree node from an object using its bounding box test.
    // (Assumes that hittable::test_bb returns:
    //  - 'w' for completely outside,
    //  - 'b' for completely inside,
    //  - and any other value for a partial intersection.)
    static OctreeNode FromObject(const BoundingBox& bb, const hittable& obj, int depth_limit = 3) {
        OctreeNode root = EmptyNode();
        char test = obj.test_bb(bb);
        if (test == 'w') {
            // completely outside, empty node
            return root;
        }
        else if (test == 'b' || depth_limit == 0) {
            // fully inside or no more subdivisions allowed
            return FullNode();
        }
        else {
            // partial intersection, subdivide
            root.subdivide();
            for (int i = 0; i < 8; i++) {
                root.children[i] = FromObject(bb.subdivide(i), obj, depth_limit - 1);
            }
            root.simplify();
            return root;
        }
    }

    // Add this function to your OctreeNode class.
    void simplify() {
        // If there are no children, there's nothing to simplify.
        if (children.empty()) {
            return;
        }

        // Recursively simplify all children.
        for (auto& child : children) {
            child.simplify();
        }

        // Check if all children are leaves (no further subdivisions)
        // and have the same fill status.
        bool canMerge = true;
        bool commonStatus = children[0].is_filled;
        for (const auto& child : children) {
            // If a child has its own children or a different fill status,
            // then we cannot merge.
            if (!child.children.empty() || child.is_filled != commonStatus) {
                canMerge = false;
                break;
            }
        }

        // If all children are mergeable, update this node.
        if (canMerge) {
            // This node now takes on the common status, and the children
            // are removed. The parent's bounding box (passed externally)
            // covers all merged children.
            is_filled = commonStatus;
            children.clear();
        }
    }

    static OctreeNode FromString(const std::string& input) {
        size_t pos = 0;
        return FromStringRecursive(input, pos);
    }

    bool IsPartial() const {
        return !children.empty();
    }

    void subdivide() {
        assert(!is_filled && children.empty());
        children.resize(8, EmptyNode());
    }

    std::vector<BoundingBox> GetFilledBoundingBoxes(const BoundingBox& root_bb) const {
        std::vector<BoundingBox> result;
        if (is_filled && children.empty()) {
            result.push_back(root_bb);
        }
        else if (!children.empty()) {
            for (int i = 0; i < 8; i++) {
                auto child_bbs = children[i].GetFilledBoundingBoxes(root_bb.subdivide(i));
                result.insert(result.end(), child_bbs.begin(), child_bbs.end());
            }
        }
        return result;
    }

    // Gathers filled points from the octree; the width is computed from the bounding box dimensions.
    void GetFilledPoints(const BoundingBox& root_bb, std::vector<std::tuple<point3, double>>& points) const {
        if (is_filled) {
            double width = root_bb.getDimensions().x();
            points.emplace_back(root_bb.vmin, width);
        }
        else if (!children.empty()) {
            for (int i = 0; i < 8; ++i) {
                children[i].GetFilledPoints(root_bb.subdivide(i), points);
            }
        }
    }

    // Test whether a point is inside a filled region of the octree.
    bool TestPoint(const BoundingBox& root_bb, const point3& p) const {
        if (root_bb.contains(p)) {
            if (is_filled) {
                return true;
            }
            else if (!children.empty()) {
                // Determine which child the point falls into by comparing to the center.
                point3 moved = p - root_bb.getCenter();
                int index = (int(moved.x() > 0) + 2 * int(moved.y() > 0) + 4 * int(moved.z() > 0));
                return children[index].TestPoint(root_bb.subdivide(index), p);
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    std::string ToString() const {
        if (is_filled) {
            return "B";
        }
        if (children.empty()) {
            return "W";
        }
        std::string result = "(";
        for (const auto& child : children) {
            result += child.ToString();
        }
        return result;
    }

    // Prints the octree in a hierarchical format along with the bounding box's computed width.
    void ToHierarchicalString(std::ostream& os, const BoundingBox& root_bb, int depth = 0, const std::string& prefix = "") const {
        const std::string branch = "L__ ";
        const std::string vertical = "|   ";
        const std::string last_branch = "\\__ ";
        const std::string space = "    ";

        std::string status = is_filled ? "Filled" : (children.empty() ? "Empty" : "Partial");
        double width = root_bb.getDimensions().x();
        os << prefix << status << " (Width: " << width << ")\n";

        if (!children.empty()) {
            for (size_t i = 0; i < children.size(); ++i) {
                std::string child_prefix = (i == children.size() - 1) ? last_branch : branch;
                std::string next_level_prefix = (i == children.size() - 1) ? space : vertical;
                children[i].ToHierarchicalString(os, root_bb.subdivide(i), depth + 1, prefix + next_level_prefix + child_prefix);
            }
        }
    }

    static OctreeNode FromStringRecursive(const std::string& input, size_t& pos) {
        if (input.empty()) {
            throw std::runtime_error("Input string is empty.");
        }

        assert(pos < input.size());
        if (input[pos] == 'B') {
            pos++;
            return FullNode();
        }
        if (input[pos] == 'W') {
            pos++;
            return EmptyNode();
        }
        if (input[pos] == '(') {
            pos++;
            std::vector<OctreeNode> children(8);
            for (int i = 0; i < 8; i++) {
                children[i] = FromStringRecursive(input, pos);
            }
            return OctreeNode(false, children);
        }
        throw std::runtime_error("Invalid input format");
    }

    // Performs a boolean operation between two octree nodes.
    static OctreeNode BooleanRecursive(
        const OctreeNode& n1, const OctreeNode& n2,
        const std::string& operation)
    {
        // Both nodes are leaf nodes
        if (n1.children.empty() && n2.children.empty()) {
            bool fillA = n1.is_filled;
            bool fillB = n2.is_filled;

            if (operation == "intersection") {
                return (fillA && fillB) ? FullNode() : EmptyNode();
            }
            else if (operation == "union") {
                return (fillA || fillB) ? FullNode() : EmptyNode();
            }
            else if (operation == "difference") {
                return (fillA && !fillB) ? FullNode() : EmptyNode();
            }
        }

        // Handle cases where one node is a leaf node
        if (n1.children.empty()) {
            if (n1.is_filled) {
                if (operation == "intersection") {
                    return n2;
                }
                else if (operation == "union") {
                    return n1;
                }
                else if (operation == "difference") {
                    return InvertNode(n2);
                }
            }
            else {
                if (operation == "intersection" || operation == "difference") {
                    return EmptyNode();
                }
                else if (operation == "union") {
                    return n2;
                }
            }
        }

        if (n2.children.empty()) {
            if (n2.is_filled) {
                if (operation == "intersection") {
                    return n1;
                }
                else if (operation == "union") {
                    return n2;
                }
                else if (operation == "difference") {
                    return EmptyNode();
                }
            }
            else {
                if (operation == "intersection") {
                    return EmptyNode();
                }
                else if (operation == "union" || operation == "difference") {
                    return n1;
                }
            }
        }

        // Recursive case: Both nodes have children
        OctreeNode result;
        result.subdivide();
        for (int i = 0; i < 8; ++i) {
            result.children[i] = BooleanRecursive(
                n1.children[i], n2.children[i], operation
            );
        }

        return result;
    }

    // Inverts a node: empty becomes filled and vice-versa.
    static OctreeNode InvertNode(const OctreeNode& node)
    {
        if (node.children.empty()) {
            return node.is_filled ? OctreeNode::EmptyNode() : OctreeNode::FullNode();
        }
        OctreeNode result;
        result.subdivide();
        for (int i = 0; i < 8; ++i) {
            result.children[i] = InvertNode(node.children[i]);
        }
        return result;
    }

    // Rebuilds an octree node from a collection of filled bounding boxes.
    static OctreeNode RebuildFromFilledBbs(
        const std::vector<BoundingBox>& filledBbs,
        const BoundingBox& regionBB,
        int maxDepth,
        int currentDepth)
    {
        std::vector<BoundingBox> intersections;
        intersections.reserve(filledBbs.size());
        for (const auto& bb : filledBbs) {
            if (bb.intersects(regionBB)) {
                intersections.push_back(bb);
            }
        }

        if (intersections.empty()) {
            return OctreeNode::EmptyNode();
        }

        // Check if exactly one bounding box completely covers regionBB.
        if (intersections.size() == 1) {
            const auto& bb = intersections[0];
            if (bb.vmin.x() <= regionBB.vmin.x() &&
                bb.vmin.y() <= regionBB.vmin.y() &&
                bb.vmin.z() <= regionBB.vmin.z() &&
                bb.vmax.x() >= regionBB.vmax.x() &&
                bb.vmax.y() >= regionBB.vmax.y() &&
                bb.vmax.z() >= regionBB.vmax.z())
            {
                return OctreeNode::FullNode();
            }
        }

        if (currentDepth >= maxDepth) {
            return OctreeNode::FullNode();
        }

        // Otherwise, subdivide to handle partial coverage
        OctreeNode partial;
        partial.subdivide();
        for (int i = 0; i < 8; ++i) {
            BoundingBox childBB = regionBB.subdivide(i);
            partial.children[i] = RebuildFromFilledBbs(
                filledBbs,
                childBB,
                maxDepth,
                currentDepth + 1
            );
        }

        return partial;
    }

};

#endif // NODE_H
