#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <vector>
#include <algorithm>
#include "hittable.h"
#include "ray.h"

class BoundingBox {
public:
    point3 vmin;
    point3 vmax;

    // Default constructor - creates a unit box at origin
    BoundingBox() {
        set_infinite_negative();
    }

    // Main constructor for rectangular box
    BoundingBox(const point3& min_corner, const point3& max_corner)
        : vmin(min_corner), vmax(max_corner) {
    }

    // Alternate constructor for cubic box
    BoundingBox(const point3& corner, double width)
        : BoundingBox(corner, corner + point3(width, width, width)) {
    }

    // Equality operators
    bool operator==(const BoundingBox& other) const {
        return (vmin == other.vmin && vmax == other.vmax);
    }

    bool operator!=(const BoundingBox& other) const {
        return (vmax != other.vmax) || (vmin != other.vmin);
    }

    // Geometry functions
    point3 getCenter() const {
        return vmin + (vmax - vmin) * 0.5;
    }

    point3 getDimensions() const {
        return vmax - vmin;
    }

    void set_infinite_negative() {
        vmin = point3(std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity());
        vmax = point3(-std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity());
    }

    void setCorner(const point3& new_corner) {
        point3 size = getDimensions();
        vmin = new_corner;
        vmax = vmin + size;
    }

    double getVolume() const {
        point3 size = getDimensions();
        return size.x() * size.y() * size.z();
    }

    double getSurfaceArea() const {
        point3 size = getDimensions();
        return 2.0 * (size.x() * size.y() + size.y() * size.z() + size.z() * size.x());
    }

    std::vector<point3> getVertices() const {
        std::vector<point3> verts;
        verts.reserve(8);
        int coords[8][3] = {
            {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
            {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}
        };
        point3 size = getDimensions();
        for (int i = 0; i < 8; i++) {
            verts.push_back(vmin + point3(
                coords[i][0] * size.x(),
                coords[i][1] * size.y(),
                coords[i][2] * size.z()
            ));
        }
        return verts;
    }

    BoundingBox subdivide(int i) const {
        int dir_x = i % 2;
        int dir_y = (i / 2) % 2;
        int dir_z = (i / 4) % 2;

        point3 size = getDimensions() * 0.5;
        point3 new_corner = vmin + point3(
            dir_x * size.x(),
            dir_y * size.y(),
            dir_z * size.z()
        );

        return BoundingBox(new_corner, new_corner + size);
    }

    point3 getClosestPoint(const point3& p) const {
        double nx = std::max(vmin.x(), std::min(p.x(), vmax.x()));
        double ny = std::max(vmin.y(), std::min(p.y(), vmax.y()));
        double nz = std::max(vmin.z(), std::min(p.z(), vmax.z()));
        return point3(nx, ny, nz);
    }

    point3 getFurthestPoint(const point3& p) const {
        auto verts = getVertices();
        point3 furthest = verts[0];
        double max_dist = (p - furthest).length();
        for (size_t i = 1; i < verts.size(); i++) {
            double d = (p - verts[i]).length();
            if (d > max_dist) {
                max_dist = d;
                furthest = verts[i];
            }
        }
        return furthest;
    }

    bool contains(const point3& p) const {
        return (p.x() >= vmin.x() && p.x() <= vmax.x() &&
            p.y() >= vmin.y() && p.y() <= vmax.y() &&
            p.z() >= vmin.z() && p.z() <= vmax.z());
    }

    bool intersects(const BoundingBox& other) const {
        return !(vmax.x() < other.vmin.x() || vmin.x() > other.vmax.x() ||
            vmax.y() < other.vmin.y() || vmin.y() > other.vmax.y() ||
            vmax.z() < other.vmin.z() || vmin.z() > other.vmax.z());
    }

    BoundingBox enclose(const BoundingBox& other) const {
        point3 new_vmin(
            std::min(vmin.x(), other.vmin.x()),
            std::min(vmin.y(), other.vmin.y()),
            std::min(vmin.z(), other.vmin.z())
        );

        point3 new_vmax(
            std::max(vmax.x(), other.vmax.x()),
            std::max(vmax.y(), other.vmax.y()),
            std::max(vmax.z(), other.vmax.z())
        );

        return BoundingBox(new_vmin, new_vmax);
    }

    void include(const point3& p) {
        vmin = point3(std::min(vmin.x(), p.x()),
            std::min(vmin.y(), p.y()),
            std::min(vmin.z(), p.z()));

        vmax = point3(std::max(vmax.x(), p.x()),
            std::max(vmax.y(), p.y()),
            std::max(vmax.z(), p.z()));
    }

    BoundingBox from_intersect(const BoundingBox& other) const {
        point3 new_vmin(
            std::max(vmin.x(), other.vmin.x()),
            std::max(vmin.y(), other.vmin.y()),
            std::max(vmin.z(), other.vmin.z())
        );

        point3 new_vmax(
            std::min(vmax.x(), other.vmax.x()),
            std::min(vmax.y(), other.vmax.y()),
            std::min(vmax.z(), other.vmax.z())
        );
        return BoundingBox(new_vmin, new_vmax);
    }

    // Hit function to test ray intersection
    bool hit(const ray& r, interval ray_t = interval(0.0, std::numeric_limits<double>::infinity())) const {
        interval temp_t = ray_t;

        for (int i = 0; i < 3; i++) {
            double invD = 1.0 / r.direction()[i];
            double t0 = (vmin[i] - r.origin()[i]) * invD;
            double t1 = (vmax[i] - r.origin()[i]) * invD;

            if (invD < 0.0) {
                std::swap(t0, t1);
            }

            temp_t.min = std::max(t0, temp_t.min);
            temp_t.max = std::min(t1, temp_t.max);

            if (temp_t.max <= temp_t.min) {
                return false;
            }
        }
        return true;
    }


    BoundingBox bounding_box() const {
        return *this;
    }
};

#endif // BOUNDINGBOX_H
