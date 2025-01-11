#ifndef SQUAREPYRAMID_H
#define SQUAREPYRAMID_H

#include "hittable.h"
#include "vec3.h"
#include "boundingbox.h"
#include <cmath>

class SquarePyramid {
public:
    point3 inferiorPoint; // Bottom center of the pyramid's base
    double height;        // Height of the pyramid
    double basis;         // Length of the base square's side

    // Constructor
    SquarePyramid(const point3& inferiorPoint, double height, double basis)
        : inferiorPoint(inferiorPoint), height(height), basis(basis) {
    }

    // Bounding Box for the Square Pyramid
    BoundingBox boundingBox() const {
        point3 min_corner = inferiorPoint - point3(basis / 2.0, 0, basis / 2.0);
        double max_dimension = std::max(basis, height);
        return BoundingBox(min_corner, max_dimension);
    }

    // Volume of the pyramid
    double volume() const {
        return (basis * basis * height) / 3.0;
    }

    // Helper that determines if a point is inside the SquarePyramid
    bool is_point_inside_pyramid(const point3& p) const {
        // Check vertical range
        if (p.y() < inferiorPoint.y() || p.y() > (inferiorPoint.y() + height)) {
            return false;
        }

        // Calculate how wide the pyramid is at the 'p.y()' level
        double distanceFromTop = (inferiorPoint.y() + height) - p.y();
        double proportionalBasis = basis * (distanceFromTop / height);
        double halfSide = proportionalBasis / 2.0;

        double minX = inferiorPoint.x() - halfSide;
        double maxX = inferiorPoint.x() + halfSide;
        if (p.x() < minX || p.x() > maxX) {
            return false;
        }

        double minZ = inferiorPoint.z() - halfSide;
        double maxZ = inferiorPoint.z() + halfSide;
        if (p.z() < minZ || p.z() > maxZ) {
            return false;
        }

        // If all checks pass, the point is inside the pyramid
        return true;
    }


    // Intersection test with a bounding box
    //char test_bb(const BoundingBox& bb) const override {
    //    // Step 1: Quick bounding-box overlap check
    //    //         If the pyramid's bounding box and 'bb' do not intersect, return 'w'.
    //    if (!bb.Intersects(this->boundingBox())) {
    //        return 'w'; // completely outside
    //    }

    //    // Step 2a: Count how many corners of 'bb' are inside the pyramid.
    //    unsigned int bbCornersInsidePyramid = 0;
    //    for (const auto& corner : bb.Vertices()) {
    //        if (is_point_inside_pyramid(corner)) {
    //            bbCornersInsidePyramid++;
    //        }
    //    }

    //    // Step 2b: Count how many corners of the pyramid's bounding box are inside 'bb'.
    //    unsigned int pyramidBBCornersInsideBB = 0;
    //    auto pyramidBBVertices = this->boundingBox().Vertices();
    //    for (const auto& corner : pyramidBBVertices) {
    //        // The bounding box class has a TestPoint(...) function.
    //        if (bb.TestPoint(corner)) {
    //            pyramidBBCornersInsideBB++;
    //        }
    //    }

    //    // Decision logic:
    //    // 1) If all corners of 'bb' are inside the pyramid => 'b' (completely inside).
    //    if (bbCornersInsidePyramid == 8) {
    //        return 'b';
    //    }

    //    // 2) If more than one corner of the pyramid’s bounding box is inside 'bb' => 'g' (partial).
    //    if (pyramidBBCornersInsideBB > 1) {
    //        return 'g';
    //    }

    //    // 3) If at least one corner of 'bb' is inside the pyramid => 'g' (partial).
    //    if (bbCornersInsidePyramid > 0) {
    //        return 'g';
    //    }

    //    // 4) Otherwise => 'w' (completely outside).
    //    return 'w';
    //}


};

#endif // SQUAREPYRAMID_H