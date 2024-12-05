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

    static Node Empty() {
        return Node(false, {});
    }

    static Node Full() {
        return Node(true, {});
    }

    static Node FromObject(const BoundingBox& bb, const sphere& obj, int depth_limit = 10) {
        Node root = Empty();
        char test = obj.test_bb(bb);
        if (test == 'w') {
            // completely outside, empty node
            return root;
        }
        else if (test == 'b' || depth_limit == 0) {
            // fully inside or no more subdivisions allowed
            root.is_filled = true;
            return root;
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
        children.resize(8, Empty());
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
};

#endif // NODE_H
