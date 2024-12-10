#ifndef NODE_H
#define NODE_H

#include <vector>
#include <cassert>
#include "boundingBox.h"
#include "sphere.h"

class Node {
public:
    bool is_filled;
    std::vector<Node> children; // size 0 or 8

    Node(bool filled, const std::vector<Node>& ch = {})
        : is_filled(filled), children(ch) {
    }

    static Node EmptyNode() {
        return Node(false, {});
    }

    static Node FullNode() {
        return Node(true, {});
    }

    static Node FromObject(const BoundingBox& bb, const sphere& obj, int depth_limit = 10) {
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


};

#endif // NODE_H
