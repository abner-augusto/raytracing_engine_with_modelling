#ifndef SQUAREPYRAMID_H
#define SQUAREPYRAMID_H

#include "hittable.h"
#include "vec3.h"
#include "boundingbox.h"
#include <cmath>

constexpr double EPSILON = 1e-8;

class SquarePyramid : public hittable {
public:
    point3 inferiorPoint; // Center of the base (lying in the XZ plane)
    double height;        // Height of the pyramid (from base to apex)
    double basis;         // Side length of the square base
    mat material;         // Material of the pyramid

    // Constructor now takes a material.
    SquarePyramid(const point3& inferiorPoint, double height, double basis, const mat& material)
        : inferiorPoint(inferiorPoint), height(height), basis(basis), material(material) {
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
    bool is_point_inside(const point3& p) const override {
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

    // Tests the ray against the pyramid’s base and its 4 lateral triangular faces.
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        bool hit_found = false;
        double closest_t = ray_t.max;
        hit_record temp_rec;

        // --- 1. Test the base (a horizontal square) ---
        if (std::fabs(r.direction().y()) > EPSILON) {
            double t_base = (inferiorPoint.y() - r.origin().y()) / r.direction().y();
            if (t_base >= ray_t.min && t_base <= ray_t.max) {
                point3 p_base = r.at(t_base);
                double half = basis / 2.0;
                if (p_base.x() >= inferiorPoint.x() - half && p_base.x() <= inferiorPoint.x() + half &&
                    p_base.z() >= inferiorPoint.z() - half && p_base.z() <= inferiorPoint.z() + half) {
                    if (t_base < closest_t) {
                        closest_t = t_base;
                        temp_rec.t = t_base;
                        temp_rec.p = p_base;
                        // The base normal points downward.
                        vec3 normal = vec3(0, -1, 0);
                        if (dot(r.direction(), normal) > 0)
                            normal = -normal;
                        temp_rec.normal = normal;
                        temp_rec.hit_object = this;
                        temp_rec.material = &material; // Pass the material to the hit record.
                        hit_found = true;
                    }
                }
            }
        }

        // --- 2. Test the lateral faces (4 triangles) ---
        point3 apex = inferiorPoint + vec3(0, height, 0);
        // Define the four base corners.
        point3 bfl(inferiorPoint.x() - basis / 2.0, inferiorPoint.y(), inferiorPoint.z() - basis / 2.0); // front-left
        point3 bfr(inferiorPoint.x() + basis / 2.0, inferiorPoint.y(), inferiorPoint.z() - basis / 2.0); // front-right
        point3 bbr(inferiorPoint.x() + basis / 2.0, inferiorPoint.y(), inferiorPoint.z() + basis / 2.0); // back-right
        point3 bbl(inferiorPoint.x() - basis / 2.0, inferiorPoint.y(), inferiorPoint.z() + basis / 2.0); // back-left

        double t;
        vec3 face_normal;
        point3 p;

        // For each triangular face, use the helper function to test intersection.
        if (ray_triangle_intersect(r, ray_t.min, ray_t.max, bfl, bfr, apex, t, face_normal, p)) {
            if (t < closest_t) {
                closest_t = t;
                temp_rec.t = t;
                temp_rec.p = p;
                vec3 interior = inferiorPoint + vec3(0, height / 2.0, 0);
                if (dot(face_normal, (p - interior)) < 0)
                    face_normal = -face_normal;
                if (dot(r.direction(), face_normal) > 0)
                    face_normal = -face_normal;
                temp_rec.normal = face_normal;
                temp_rec.hit_object = this;
                temp_rec.material = &material;
                hit_found = true;
            }
        }
        if (ray_triangle_intersect(r, ray_t.min, ray_t.max, bfr, bbr, apex, t, face_normal, p)) {
            if (t < closest_t) {
                closest_t = t;
                temp_rec.t = t;
                temp_rec.p = p;
                vec3 interior = inferiorPoint + vec3(0, height / 2.0, 0);
                if (dot(face_normal, (p - interior)) < 0)
                    face_normal = -face_normal;
                if (dot(r.direction(), face_normal) > 0)
                    face_normal = -face_normal;
                temp_rec.normal = face_normal;
                temp_rec.hit_object = this;
                temp_rec.material = &material;
                hit_found = true;
            }
        }
        if (ray_triangle_intersect(r, ray_t.min, ray_t.max, bbr, bbl, apex, t, face_normal, p)) {
            if (t < closest_t) {
                closest_t = t;
                temp_rec.t = t;
                temp_rec.p = p;
                vec3 interior = inferiorPoint + vec3(0, height / 2.0, 0);
                if (dot(face_normal, (p - interior)) < 0)
                    face_normal = -face_normal;
                if (dot(r.direction(), face_normal) > 0)
                    face_normal = -face_normal;
                temp_rec.normal = face_normal;
                temp_rec.hit_object = this;
                temp_rec.material = &material;
                hit_found = true;
            }
        }
        if (ray_triangle_intersect(r, ray_t.min, ray_t.max, bbl, bfl, apex, t, face_normal, p)) {
            if (t < closest_t) {
                closest_t = t;
                temp_rec.t = t;
                temp_rec.p = p;
                vec3 interior = inferiorPoint + vec3(0, height / 2.0, 0);
                if (dot(face_normal, (p - interior)) < 0)
                    face_normal = -face_normal;
                if (dot(r.direction(), face_normal) > 0)
                    face_normal = -face_normal;
                temp_rec.normal = face_normal;
                temp_rec.hit_object = this;
                temp_rec.material = &material;
                hit_found = true;
            }
        }

        if (hit_found) {
            rec = temp_rec;
            return true;
        }
        return false;
    }

    // Compute all intersections for CSG operations.
    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override {
        out_intersections.clear();
        std::vector<CSGIntersection> candidates;

        // --- 1. Base intersection (invert normal to point outward) ---
        if (std::fabs(r.direction().y()) > EPSILON) {
            double t_base = (inferiorPoint.y() - r.origin().y()) / r.direction().y();
            if (t_base >= ray_t.min && t_base <= ray_t.max) {
                point3 p_base = r.at(t_base);
                double half = basis / 2.0;
                if (p_base.x() >= inferiorPoint.x() - half && p_base.x() <= inferiorPoint.x() + half &&
                    p_base.z() >= inferiorPoint.z() - half && p_base.z() <= inferiorPoint.z() + half) {
                    // Invert base normal to point outward
                    vec3 normal = -vec3(0, -1, 0); // Now points upward
                    // Entry = ray is entering through base (direction opposes normal)
                    bool is_entry = dot(r.direction(), normal) < 0;
                    candidates.emplace_back(t_base, is_entry, this, normal, p_base);
                }
            }
        }

        // --- 2. Lateral faces intersections (invert normals to point outward) ---
        point3 apex = inferiorPoint + vec3(0, height, 0);
        point3 bfl(inferiorPoint.x() - basis / 2, inferiorPoint.y(), inferiorPoint.z() - basis / 2);
        point3 bfr(inferiorPoint.x() + basis / 2, inferiorPoint.y(), inferiorPoint.z() - basis / 2);
        point3 bbr(inferiorPoint.x() + basis / 2, inferiorPoint.y(), inferiorPoint.z() + basis / 2);
        point3 bbl(inferiorPoint.x() - basis / 2, inferiorPoint.y(), inferiorPoint.z() + basis / 2);

        // Helper to process each triangular face
        auto process_face = [&](const point3& v0, const point3& v1, const point3& v2) {
            double t;
            vec3 face_normal;
            point3 p;
            if (ray_triangle_intersect(r, ray_t.min, ray_t.max, v0, v1, v2, t, face_normal, p)) {
                // Ensure normal points outward using interior test
                vec3 interior = inferiorPoint + vec3(0, height / 2, 0);
                if (dot(face_normal, p - interior) > 0) {
                    face_normal = -face_normal; // Flip if pointing inward
                }
                // Invert the normal to point outward
                face_normal = -face_normal;
                // Determine entry using outward-facing normal
                bool is_entry = dot(r.direction(), face_normal) < 0;
                candidates.emplace_back(t, is_entry, this, face_normal, p);
            }
            };

        // Process all four lateral faces
        process_face(bfl, bfr, apex);  // Front face
        process_face(bfr, bbr, apex);  // Right face
        process_face(bbr, bbl, apex);  // Back face
        process_face(bbl, bfl, apex);  // Left face

        // --- 3. Sort and deduplicate intersections ---
        std::sort(candidates.begin(), candidates.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });

        // Remove near-identical hits (accounts for floating point precision)
        const double epsilon = 1e-6;
        auto last = std::unique(candidates.begin(), candidates.end(),
            [epsilon](const CSGIntersection& a, const CSGIntersection& b) {
                return std::abs(a.t - b.t) < epsilon &&
                    dot(a.normal, b.normal) > 0.999; // Similar normals
            });
        candidates.erase(last, candidates.end());

        // Final validation of entry/exit sequence
        bool inside = is_point_inside(r.origin());
        for (auto& inter : candidates) {
            inter.is_entry = !inside;
            inside = !inside; // Toggle state after each intersection
        }

        out_intersections = candidates;
        return !out_intersections.empty();
    }

    // Intersection test with a bounding box
    char test_bb(const BoundingBox& bb) const override {
        // Step 1: Quick bounding-box overlap check
        //         If the pyramid's bounding box and 'bb' do not intersect, return 'w'.
        if (!bb.intersects(this->boundingBox())) {
            return 'w'; // completely outside
        }

        // Step 2a: Count how many corners of 'bb' are inside the pyramid.
        unsigned int bbCornersInsidePyramid = 0;
        for (const auto& corner : bb.getVertices()) {
            if (is_point_inside(corner)) {
                bbCornersInsidePyramid++;
            }
        }

        // Step 2b: Count how many corners of the pyramid's bounding box are inside 'bb'.
        unsigned int pyramidBBCornersInsideBB = 0;
        auto pyramidBBVertices = this->boundingBox().getVertices();
        for (const auto& corner : pyramidBBVertices) {
            // The bounding box class has a TestPoint(...) function.
            if (bb.contains(corner)) {
                pyramidBBCornersInsideBB++;
            }
        }

        // Decision logic:
        // 1) If all corners of 'bb' are inside the pyramid => 'b' (completely inside).
        if (bbCornersInsidePyramid == 8) {
            return 'b';
        }

        // 2) If more than one corner of the pyramid’s bounding box is inside 'bb' => 'g' (partial).
        if (pyramidBBCornersInsideBB > 1) {
            return 'g';
        }

        // 3) If at least one corner of 'bb' is inside the pyramid => 'g' (partial).
        if (bbCornersInsidePyramid > 0) {
            return 'g';
        }

        // 4) Otherwise => 'w' (completely outside).
        return 'w';
    }

    // Returns an axis-aligned bounding box that tightly encloses the pyramid.
    BoundingBox bounding_box() const override {
        point3 min_corner(inferiorPoint.x() - basis / 2.0,
            inferiorPoint.y(),
            inferiorPoint.z() - basis / 2.0);
        point3 max_corner(inferiorPoint.x() + basis / 2.0,
            inferiorPoint.y() + height,
            inferiorPoint.z() + basis / 2.0);
        return BoundingBox(min_corner, max_corner);
    }

    void transform(const Matrix4x4& matrix) override {
        // Apply transformation to the center of the sphere
        inferiorPoint = matrix.transform_point(inferiorPoint);
        double scalingFactor = matrix.get_uniform_scale();
        basis *= scalingFactor;
        height *= scalingFactor;
    }

    std::string get_type_name() const override {
        return "SquarePyramid";
    }

    private:
        // Helper: Ray–triangle intersection using the Möller–Trumbore algorithm.
        // Returns true if the ray intersects the triangle (v0, v1, v2) within [t_min, t_max].
        static bool ray_triangle_intersect(const ray& r, double t_min, double t_max,
            const point3& v0, const point3& v1, const point3& v2,
            double& t, vec3& normal, point3& p) {
            vec3 edge1 = v1 - v0;
            vec3 edge2 = v2 - v0;
            vec3 h = cross(r.direction(), edge2);
            double a = dot(edge1, h);
            if (std::fabs(a) < EPSILON)
                return false; // Ray is parallel to the triangle.
            double f = 1.0 / a;
            vec3 s = r.origin() - v0;
            double u = f * dot(s, h);
            if (u < 0.0 || u > 1.0)
                return false;
            vec3 q = cross(s, edge1);
            double v = f * dot(r.direction(), q);
            if (v < 0.0 || u + v > 1.0)
                return false;
            t = f * dot(edge2, q);
            if (t < t_min || t > t_max)
                return false;
            p = r.at(t);
            normal = unit_vector(cross(edge1, edge2));
            return true;
        }

};

#endif // SQUAREPYRAMID_H