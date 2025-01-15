#ifndef NODE_H
#define NODE_H

#include <vector>
#include <cassert>
#include "boundingbox.h"
#include "primitive.h"

class Node {
public:
    bool is_filled;
    std::vector<Node> children; // size 0 or 8

    // Default constructor
    Node() : is_filled(false), children() {}

    Node(bool filled, const std::vector<Node>& ch = {})
        : is_filled(filled), children(ch) {
    }

    static Node EmptyNode() {
        return Node(false, {});
    }

    static Node FullNode() {
        return Node(true, {});
    }

    static Node FromObject(const BoundingBox& bb, const Primitive& obj, int depth_limit = 3) {
        Node root = EmptyNode();
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
            root.Subdivide();
            for (int i = 0; i < 8; i++) {
                root.children[i] = FromObject(bb.Subdivide(i), obj, depth_limit - 1);
            }
            return root;
        }
    }

    static Node FromString(const std::string& input) {
        size_t pos = 0;
        return FromStringRecursive(input, pos);
    }

    bool IsPartial() const {
        return !children.empty();
    }

    void Subdivide() {
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
                auto child_bbs = children[i].GetFilledBoundingBoxes(root_bb.Subdivide(i));
                result.insert(result.end(), child_bbs.begin(), child_bbs.end());
            }
        }
        return result;
    }

    void GetFilledPoints(const BoundingBox& root_bb, std::vector<std::tuple<point3, double>>& points) const {
        if (is_filled) {
            // Add the minimum corner and width of the bounding box
            points.emplace_back(root_bb.vmin, root_bb.width);
        }
        else if (!children.empty()) {
            // Recurse into children
            for (int i = 0; i < 8; ++i) {
                children[i].GetFilledPoints(root_bb.Subdivide(i), points);
            }
        }
    }

    bool TestPoint(const BoundingBox& root_bb, const point3& p) const {
        if (root_bb.TestPoint(p)) {
            if (is_filled) {
                return true;
            }
            else if (!children.empty()) {
                point3 moved = p - root_bb.Center();
                int index = (int(moved.x() > 0) + 2 * int(moved.y() > 0) + 4 * int(moved.z() > 0));
                return children[index].TestPoint(root_bb.Subdivide(index), p);
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

    void ToHierarchicalString(std::ostream& os, const BoundingBox& root_bb, int depth = 0, const std::string& prefix = "") const {
        const std::string branch = "L__ ";
        const std::string vertical = "|   ";
        const std::string last_branch = "\\__ ";
        const std::string space = "    ";

        // Identify the node's status
        std::string status = is_filled ? "Filled" : (children.empty() ? "Empty" : "Partial");

        // Include bounding box width
        std::string bb_width = std::to_string(root_bb.width);

        // Print the prefix, status, and bounding box width
        os << prefix << status << " (Width: " << bb_width << ")\n";

        // Process children
        if (!children.empty()) {
            for (size_t i = 0; i < children.size(); ++i) {
                std::string child_prefix = (i == children.size() - 1) ? last_branch : branch;
                std::string next_level_prefix = (i == children.size() - 1) ? space : vertical;
                children[i].ToHierarchicalString(os, root_bb.Subdivide(i), depth + 1, prefix + next_level_prefix + child_prefix);
            }
        }
    }

    static Node FromStringRecursive(const std::string& input, size_t& pos) {
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
            std::vector<Node> children(8);
            for (int i = 0; i < 8; i++) {
                children[i] = FromStringRecursive(input, pos);
            }
            return Node(false, children);
        }
        throw std::runtime_error("Invalid input format");
    }
    static Node BooleanRecursive(
        const Node& n1, const Node& n2,
        const std::string& operation)
    {
        // Handle cases where both nodes are leaf nodes (no children)
        if (n1.children.empty() && n2.children.empty()) {
            // Extract the filled state of the leaf nodes
            bool fillA = n1.is_filled;
            bool fillB = n2.is_filled;

            // Perform the specified boolean operation for leaf nodes
            if (operation == "intersection") {
                // Intersection: Both nodes must be filled
                return (fillA && fillB) ? FullNode() : EmptyNode();
            }
            else if (operation == "union") {
                // Union: At least one node must be filled
                return (fillA || fillB) ? FullNode() : EmptyNode();
            }
            else if (operation == "difference") {
                // Difference: First node filled, second node not filled
                return (fillA && !fillB) ? FullNode() : EmptyNode();
            }
        }

        // Handle cases where one node is a leaf node
        if (n1.children.empty()) {
            if (n1.is_filled) {
                // If the first node is a filled leaf
                if (operation == "intersection") {
                    // Intersection: Result is the second node
                    return n2;
                }
                else if (operation == "union") {
                    // Union: Result is the filled first node
                    return n1;
                }
                else if (operation == "difference") {
                    // Difference: Invert the second node
                    return InvertNode(n2);
                }
            }
            else { // The first node is an empty leaf
                if (operation == "intersection" || operation == "difference") {
                    // Intersection or difference with an empty node yields an empty node
                    return EmptyNode();
                }
                else if (operation == "union") {
                    // Union with an empty node yields the second node
                    return n2;
                }
            }
        }

        if (n2.children.empty()) {
            if (n2.is_filled) {
                // If the second node is a filled leaf
                if (operation == "intersection") {
                    // Intersection: Result is the first node
                    return n1;
                }
                else if (operation == "union") {
                    // Union: Result is the filled second node
                    return n2;
                }
                else if (operation == "difference") {
                    // Difference: Empty result
                    return EmptyNode();
                }
            }
            else { // The second node is an empty leaf
                if (operation == "intersection") {
                    // Intersection with an empty node yields an empty node
                    return EmptyNode();
                }
                else if (operation == "union" || operation == "difference") {
                    // Union or difference with an empty node yields the first node
                    return n1;
                }
            }
        }

        // Recursive case: Both nodes have children
        Node result;
        result.Subdivide(); // Subdivide the result node to match child structure

        // Recurse through all child nodes
        for (int i = 0; i < 8; ++i) {
            result.children[i] = BooleanRecursive(
                n1.children[i], n2.children[i], operation
            );
        }

        // Return the constructed result node
        return result;
    }


    static Node InvertNode(const Node& node)
    {
        if (node.children.empty()) {
            return node.is_filled ? Node::EmptyNode() : Node::FullNode();
        }
        Node result;
        result.Subdivide();
        for (int i = 0; i < 8; ++i) {
            result.children[i] = InvertNode(node.children[i]);
        }
        return result;
    }

    static Node RebuildFromFilledBbs(
        const std::vector<BoundingBox>& filledBbs,
        const BoundingBox& regionBB,
        int maxDepth,
        int currentDepth)
    {
        std::vector<BoundingBox> intersections;
        intersections.reserve(filledBbs.size());
        for (const auto& bb : filledBbs) {
            if (bb.Intersects(regionBB)) {
                intersections.push_back(bb);
            }
        }

        if (intersections.empty()) {
            return Node::EmptyNode();
        }

        // Check if exactly one bounding box completely covers regionBB
        if (intersections.size() == 1) {
            // "Covers" means the intersection's vmin <= regionBB.vmin
            // and intersection's vmax() >= regionBB.vmax().
            // If so, we can mark this node full without subdividing.
            const auto& bb = intersections[0];
            if (bb.vmin.x() <= regionBB.vmin.x() &&
                bb.vmin.y() <= regionBB.vmin.y() &&
                bb.vmin.z() <= regionBB.vmin.z() &&
                bb.vmax().x() >= regionBB.vmax().x() &&
                bb.vmax().y() >= regionBB.vmax().y() &&
                bb.vmax().z() >= regionBB.vmax().z())
            {
                // The region is fully covered by this bounding box
                return Node::FullNode();
            }
        }

        if (currentDepth >= maxDepth) {
            return Node::FullNode();
        }

        // Otherwise, subdivide to handle partial coverage
        Node partial;
        partial.Subdivide();
        for (int i = 0; i < 8; ++i) {
            BoundingBox childBB = regionBB.Subdivide(i);
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
