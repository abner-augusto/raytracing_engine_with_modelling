#ifndef NODE_H
#define NODE_H

#include <vector>
#include <cassert>
#include "boundingbox.h"
#include "sphere.h"

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

    static Node FromObject(const BoundingBox& bb, const sphere& obj, int depth_limit = 5) {
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
        if (is_filled) {
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

    void ToHierarchicalString(std::ostream& os, int depth = 0, const std::string& prefix = "") const {
        // Define ASCII replacements for Unicode characters
        const std::string branch = "L__ ";
        const std::string vertical = "|   ";
        const std::string last_branch = "\\__ ";
        const std::string space = "    ";

        // Identify the node's status
        std::string status = is_filled ? "Filled" : (children.empty() ? "Empty" : "Partial");

        // Print the prefix and status
        os << prefix << status << "\n";

        // Process children
        if (!children.empty()) {
            for (size_t i = 0; i < children.size(); ++i) {
                std::string child_prefix = (i == children.size() - 1) ? last_branch : branch;
                std::string next_level_prefix = (i == children.size() - 1) ? space : vertical;
                children[i].ToHierarchicalString(os, depth + 1, prefix + next_level_prefix + child_prefix);
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

    static Node BooleanRecursive(const Node& n1, const BoundingBox& bb1,
        const Node& n2, const BoundingBox& bb2,
        const BoundingBox& result_bb,
        const std::string& operation) {
        // Align input bounding boxes with the result bounding box
        BoundingBox aligned_bb1 = AlignBoundingBox(bb1, result_bb);
        BoundingBox aligned_bb2 = AlignBoundingBox(bb2, result_bb);

        // Case 1: Both nodes are leaves
        if (n1.children.empty() && n2.children.empty()) {
            if (operation == "intersection") {
                return (n1.is_filled && n2.is_filled) ? FullNode() : EmptyNode();
            }
            else if (operation == "union") {
                return (n1.is_filled || n2.is_filled) ? FullNode() : EmptyNode();
            }
            else if (operation == "difference") {
                return (n1.is_filled && !n2.is_filled) ? FullNode() : EmptyNode();
            }
        }

        // Case 2: One node is a leaf
        if (n1.children.empty()) {
            return HandleLeafAndSubtree(n1, aligned_bb1, n2, aligned_bb2, result_bb, operation);
        }
        if (n2.children.empty()) {
            return HandleLeafAndSubtree(n2, aligned_bb2, n1, aligned_bb1, result_bb, operation, true);
        }

        // Case 3: Both nodes have children
        Node result;
        result.Subdivide();
        for (int i = 0; i < 8; ++i) {
            BoundingBox child_bb = result_bb.Subdivide(i);
            result.children[i] = BooleanRecursive(
                n1.children[i], aligned_bb1.Subdivide(i),
                n2.children[i], aligned_bb2.Subdivide(i),
                child_bb, operation);
        }
        return result;
    }

    // Align input bounding box to match the target bounding box's size and position
    static BoundingBox AlignBoundingBox(const BoundingBox& input_bb, const BoundingBox& target_bb) {
        return BoundingBox(target_bb.vmin, target_bb.width);
    }

    static Node HandleLeafAndSubtree(const Node& leaf, const BoundingBox& bb_leaf,
        const Node& subtree, const BoundingBox& bb_subtree,
        const BoundingBox& combined_bb,
        const std::string& operation,
        bool is_left_leaf = true)
    {
        // If the leaf is filled or empty:
        bool leaf_is_filled = leaf.is_filled;

        if (operation == "intersection") {
            // intersection(A, B) = B if A is full
            // intersection(A, B) = empty if A is empty
            if (!leaf_is_filled) {
                return EmptyNode();
            }
            else {
                // If leaf is full, the result is whatever 'subtree' is
                return subtree;
            }
        }
        else if (operation == "union") {
            // union(A, B) = B if A is empty
            // union(A, B) = full if A is full
            if (leaf_is_filled) {
                return FullNode();
            }
            else {
                return subtree;
            }
        }
        else if (operation == "difference") {
            // difference(A, B) = A - B
            // If A is empty, the result is empty
            // If A is full, the result is "full minus subtree" => invert the subtree
            if (!leaf_is_filled) {
                // empty - anything = empty
                return EmptyNode();
            }
            else {
                // full - subtree => invert(subtree)
                return InvertNode(subtree);
            }
        }
        // Fallback
        return EmptyNode();
    }

    static Node InvertNode(const Node& node)
    {
        // If it's a leaf
        if (node.children.empty()) {
            // If it was filled, return empty
            // If it was empty, return full
            return node.is_filled ? Node::EmptyNode() : Node::FullNode();
        }
        // Otherwise, invert each child
        Node result;
        result.Subdivide();
        for (int i = 0; i < 8; ++i) {
            result.children[i] = InvertNode(node.children[i]);
        }
        return result;
    }

};



#endif // NODE_H
