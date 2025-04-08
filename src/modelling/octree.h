#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <sstream>

#include "node.h"
#include "boundingbox.h"
#include "sphere.h"
#include "vec3.h"


class Octree {
public:
    BoundingBox bounding_box;
    OctreeNode root;

    // Default constructor
    Octree() : bounding_box(), root() {}
    // Parameterized constructor
    Octree(const BoundingBox& bb, const OctreeNode& r) : bounding_box(bb), root(r) {}

    static Octree FromObject(const BoundingBox& bb, const hittable& obj, int depth_limit = 3) {
        OctreeNode root = OctreeNode::FromObject(bb, obj, depth_limit);
        root.PostProcessMerge();
        return Octree(bb, root);
    }

    static Octree FromString(const BoundingBox& bb, const std::string& input) {
        size_t pos = 0;
        OctreeNode root = OctreeNode::FromStringRecursive(input, pos);
        return Octree(bb, root);
    }

    static Octree FromBooleanOperation(const Octree& octree1,
        const Octree& octree2,
        const std::string& operation,
        int maxDepth)
    {
        // Check if the bounding boxes are not aligned
        if (octree1.bounding_box != octree2.bounding_box) {
            BoundingBox mergedBB = octree1.bounding_box.enclose(octree2.bounding_box);

            // Rebuild each old octree to use the merged bounding box
            Octree rebuilt1 = RebuildOctreeFromBbs(octree1, mergedBB, maxDepth);
            Octree rebuilt2 = RebuildOctreeFromBbs(octree2, mergedBB, maxDepth);

            // Perform the boolean operation using the merged bounding box
            OctreeNode newRoot = OctreeNode::BooleanRecursive(
                rebuilt1.root,
                rebuilt2.root,
                operation
            );

            return Octree(mergedBB, newRoot);
        }

        // Bounding boxes are aligned, perform the boolean operation directly
        OctreeNode newRoot = OctreeNode::BooleanRecursive(
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
        OctreeNode newRoot = OctreeNode::RebuildFromFilledBbs(
            oldFilledBbs,
            newBB,
            maxDepth,
            0
        );

        return Octree(newBB, newRoot);
    }

    double volume() const {
        double totalVolume = 0.0;
        std::vector<BoundingBox> filledBbs = GetFilledBoundingBoxes();

        for (const auto& bb : filledBbs) {
            totalVolume += bb.getVolume();
        }

        return totalVolume;
    }

    std::vector<std::tuple<point3, double>> GetFilledPoints() const {
        std::vector<std::tuple<point3, double>> points;
        root.GetFilledPoints(bounding_box, points);
        return points;
    }

    // Improved version of CalculateHullSurfaceArea using BoundingBox methods and efficient neighbor lookup.
    double CalculateHullSurfaceArea() const {
        // Retrieve filled bounding boxes (cells)
        const auto filledBbs = GetFilledBoundingBoxes();

        // Build a hash set of cell positions (represented as a string key)
        std::unordered_set<std::string> filledSet;
        auto makeKey = [](const point3& p) -> std::string {
            std::ostringstream oss;
            // Use fixed precision to avoid floating point issues (cells align exactly)
            oss.precision(10);
            oss << p.x() << "_" << p.y() << "_" << p.z();
            return oss.str();
            };

        for (const auto& bb : filledBbs) {
            filledSet.insert(makeKey(bb.vmin));
        }

        // Directions corresponding to each face (neighbors in 6 cardinal directions)
        const std::vector<point3> directions = {
            point3(1, 0, 0), point3(-1, 0, 0),
            point3(0, 1, 0), point3(0, -1, 0),
            point3(0, 0, 1), point3(0, 0, -1)
        };

        double total_surface_area = 0.0;
        // For each filled cell, check each face for exposure.
        for (const auto& bb : filledBbs) {
            // Using getDimensions() from BoundingBox
            double cellWidth = bb.getDimensions().x();  // assuming a cube cell
            double face_area = cellWidth * cellWidth;

            for (const auto& dir : directions) {
                // Calculate the vmin of the neighboring cell by offsetting by cellWidth.
                point3 neighbor_vmin = bb.vmin + dir * cellWidth;
                if (filledSet.find(makeKey(neighbor_vmin)) == filledSet.end()) {
                    // No neighbor in that direction: face is exposed.
                    total_surface_area += face_area;
                }
            }
        }
        return total_surface_area;
    }

};

#endif // OCTREE_H
