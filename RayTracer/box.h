#ifndef BOX_H
#define BOX_H

#include <vector>
#include "hittable.h"
#include "triangle.h"
#include "material.h"
#include "vec3.h"
#include "matrix4x4.h"
#include "boundingbox.h"

class box : public hittable {
public:
    // Constructor: specify min and max corners with UV scaling
    box(const point3& _vmin, const point3& _vmax, const mat& _material, double uv_scale = 1.0)
        : vmin(_vmin), vmax(_vmax), material(_material), uv_scale(uv_scale) {
        create_box_triangles();
    }

    // Constructor: specify center and width (cube) with UV scaling
    box(const point3& center, double width, const mat& _material, double uv_scale = 1.0)
        : material(_material), uv_scale(uv_scale) {
        double half_width = width * 0.5;
        vmin = point3(center.x() - half_width, center.y() - half_width, center.z() - half_width);
        vmax = point3(center.x() + half_width, center.y() + half_width, center.z() + half_width);
        create_box_triangles();
    }

    // Constructor: specify vmin and dimensions with UV scaling
    box(const point3& _vmin, double width, double height, double depth, const mat& _material, double uv_scale = 1.0)
        : vmin(_vmin), material(_material), uv_scale(uv_scale) {
        vmax = point3(_vmin.x() + width, _vmin.y() + height, _vmin.z() + depth);
        create_box_triangles();
    }

    // Apply a transformation matrix to the entire box
    void transform(const Matrix4x4& matrix) override {
        // Transform the corner points
        vmin = matrix.transform_point(vmin);
        vmax = matrix.transform_point(vmax);

        // Transform all the internal triangles
        for (auto& tri : triangles) {
            tri->transform(matrix);
        }
    }

    // Ray hit function for the box
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        bool hit_anything = false;
        double closest_so_far = ray_t.max;

        for (const auto& tri : triangles) {
            hit_record temp_rec;
            if (tri->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    BoundingBox bounding_box() const override {
        if (triangles.empty()) {
            throw std::runtime_error("Bounding box requested for a box with no triangles.");
        }

        // Initialize min and max points with the first triangle's bounding box
        BoundingBox box = triangles[0]->bounding_box();

        // Expand the box to include all triangle bounding boxes
        for (size_t i = 1; i < triangles.size(); ++i) {
            box = box.enclose(triangles[i]->bounding_box());
        }

        return box;
    }

    std::string get_type_name() const override {
        return "Box Mesh";
    }

    // Getter for material
    mat get_material() const override {
        return material;
    }

    // Setter for material
    void set_material(const mat& new_material) override {
        material = new_material;
        for (auto& tri : triangles) {
            tri->set_material(new_material);
        }
    }

private:
    point3 vmin;
    point3 vmax;
    mat material;
    double uv_scale;
    std::vector<std::shared_ptr<triangle>> triangles;

    // Build 12 triangles for the box with consistent UV mapping and scaling
    void create_box_triangles() {
        double x0 = vmin.x();
        double y0 = vmin.y();
        double z0 = vmin.z();
        double x1 = vmax.x();
        double y1 = vmax.y();
        double z1 = vmax.z();

        // Corner points
        point3 A(x0, y0, z0);
        point3 B(x1, y0, z0);
        point3 C(x1, y1, z0);
        point3 D(x0, y1, z0);
        point3 E(x0, y0, z1);
        point3 F(x1, y0, z1);
        point3 G(x1, y1, z1);
        point3 H(x0, y1, z1);

        // Scale factor for UVs
        double us = uv_scale;
        double vs = uv_scale;

        // Add 2 triangles per face with scaled UVs
        // Front face (+Z)
        triangles.push_back(std::make_shared<triangle>(E, F, G, 0.0 * us, 0.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(E, G, H, 0.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, 0.0 * us, 1.0 * vs, material));

        // Back face (-Z)
        triangles.push_back(std::make_shared<triangle>(A, B, C, 0.0 * us, 0.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(A, C, D, 0.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, 0.0 * us, 1.0 * vs, material));

        // Right face (+X)
        triangles.push_back(std::make_shared<triangle>(F, B, G, 0.0 * us, 0.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(G, B, C, 0.0 * us, 1.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));

        // Left face (-X)
        triangles.push_back(std::make_shared<triangle>(A, E, H, 0.0 * us, 0.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(A, H, D, 0.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, 0.0 * us, 1.0 * vs, material));

        // Top face (+Y)
        triangles.push_back(std::make_shared<triangle>(C, D, G, 1.0 * us, 0.0 * vs, 0.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(G, D, H, 1.0 * us, 1.0 * vs, 0.0 * us, 0.0 * vs, 0.0 * us, 1.0 * vs, material));

        // Bottom face (-Y)
        triangles.push_back(std::make_shared<triangle>(F, E, B, 0.0 * us, 0.0 * vs, 1.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, material));
        triangles.push_back(std::make_shared<triangle>(B, E, A, 0.0 * us, 0.0 * vs, 1.0 * us, 1.0 * vs, 0.0 * us, 1.0 * vs, material));
    }

};

#endif // BOX_H