#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <vector>
#include <algorithm>
#include "vec3.h"

class BoundingBox {
public:
    point3 vmin;
    double width;


    BoundingBox(const point3& c, double w) : vmin(c), width(w) {
        update_vmax();
    }

    bool operator==(const BoundingBox& other) const {
        return (vmin == other.vmin && width == other.width);
    }

    bool operator!=(const BoundingBox& other) const {
        return (width != other.width) || (vmin != other.vmin);
    }

    point3 Center() const {
        return vmin + (width / 2.0) * point3(1, 1, 1);
    }

    const point3& vmax() const {
        return v_max;
    }

    void set_corner(const point3& new_corner) {
        vmin = new_corner;
        update_vmax();
    }

    void set_width(double new_width) {
        width = new_width;
        update_vmax();
    }

    double volume() const {
        return width * width * width;
    }

    std::vector<point3> Vertices() const {
        std::vector<point3> verts;
        verts.reserve(8);
        int coords[8][3] = {
            {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
            {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}
        };
        for (int i = 0; i < 8; i++) {
            verts.push_back(vmin + width * point3(coords[i][0], coords[i][1], coords[i][2]));
        }
        return verts;
    }

    BoundingBox Subdivide(int i) const {
        int dir_x = i % 2;
        int dir_y = (i / 2) % 2;
        int dir_z = (i / 4) % 2;
        point3 new_corner = vmin + (width / 2.0) * (
            point3(dir_x, 0, 0)
            + point3(0, dir_y, 0)
            + point3(0, 0, dir_z));
        return BoundingBox(new_corner, width / 2.0);
    }

    point3 ClosestPoint(const point3& p) const {
        double nx = std::max(vmin.x(), std::min(p.x(), v_max.x()));
        double ny = std::max(vmin.y(), std::min(p.y(), v_max.y()));
        double nz = std::max(vmin.z(), std::min(p.z(), v_max.z()));
        return point3(nx, ny, nz);
    }

    point3 FurthestPoint(const point3& p) const {
        auto verts = Vertices();
        point3 furthest = verts[0];
        double max_dist = distance(p, furthest);
        for (size_t i = 1; i < verts.size(); i++) {
            double d = distance(p, verts[i]);
            if (d > max_dist) {
                max_dist = d;
                furthest = verts[i];
            }
        }
        return furthest;
    }

    bool TestPoint(const point3& p) const {
        point3 moved = p - vmin;
        return (moved.x() >= 0 && moved.x() <= width &&
            moved.y() >= 0 && moved.y() <= width &&
            moved.z() >= 0 && moved.z() <= width);
    }

    bool Intersects(const BoundingBox& other) const {
        // Check for separation along each axis
        return !(vmax().x() < other.vmin.x() || vmin.x() > other.vmax().x() ||
            vmax().y() < other.vmin.y() || vmin.y() > other.vmax().y() ||
            vmax().z() < other.vmin.z() || vmin.z() > other.vmax().z());
    }

    // Utility to compute a cube bounding box that encloses this box and another box
    BoundingBox Enclose(const BoundingBox& other) const {
        // Get the min-corner (lowest x, y, z of both boxes)
        point3 new_vmin(
            std::min(vmin.x(), other.vmin.x()),
            std::min(vmin.y(), other.vmin.y()),
            std::min(vmin.z(), other.vmin.z())
        );

        // Get the max-corner (highest x, y, z of both boxes)
        point3 this_vmax = vmax();
        point3 other_vmax = other.vmax();

        point3 new_vmax(
            std::max(this_vmax.x(), other_vmax.x()),
            std::max(this_vmax.y(), other_vmax.y()),
            std::max(this_vmax.z(), other_vmax.z())
        );

        // Determine the range in x, y, z
        double range_x = new_vmax.x() - new_vmin.x();
        double range_y = new_vmax.y() - new_vmin.y();
        double range_z = new_vmax.z() - new_vmin.z();

        // For a true octree, you want a perfect cube, so pick the largest dimension
        double max_side = std::max(range_x, std::max(range_y, range_z));

        // Return a bounding box starting at new_vmin with side = max_side
        return BoundingBox(new_vmin, max_side);
    }

private:
    point3 v_max;

    void update_vmax() {
        v_max = vmin + width * point3(1, 1, 1);
    }
};

#endif // BOUNDINGBOX_H
