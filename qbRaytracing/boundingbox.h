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

private:
    point3 v_max;

    void update_vmax() {
        v_max = vmin + width * point3(1, 1, 1);
    }
};

#endif // BOUNDINGBOX_H
