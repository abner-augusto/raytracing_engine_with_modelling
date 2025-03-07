#ifndef MESH_H
#define MESH_H

#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include "vec3.h"
#include "scene.h"
#include "material.h"
#include "bvh_node.h"
#include "triangle.h"

class Mesh : public hittable {
public:
    Mesh() {}

    void add_triangle(std::shared_ptr<triangle> tri) {
        triangles.push_back(tri);
        root_bvh = nullptr; // Invalidate BVH
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (root_bvh) {
            return root_bvh->hit(r, ray_t, rec);
        }
        else {
            return defaultHitTraversal(r, ray_t, rec);
        }
    }

    void transform(const Matrix4x4& matrix) override {
        for (auto& tri : triangles) {
            tri->transform(matrix);
        }
        buildBVH(); // Rebuild BVH after transformation
    }

    BoundingBox bounding_box() const override {
        if (root_bvh) {
            return root_bvh->bounding_box();  // Use BVH bounding box if available
        }

        if (triangles.empty()) {
            return BoundingBox();
        }

        BoundingBox combined_box = triangles[0]->bounding_box();
        for (size_t i = 1; i < triangles.size(); ++i) {
            combined_box = combined_box.enclose(triangles[i]->bounding_box());
        }
        return combined_box;
    }

    std::string get_type_name() const override {
        return "Mesh";
    }

    void set_material(const mat& new_material) override {
        for (auto& tri : triangles) {
            tri->set_material(new_material);
        }
    }

    mat get_material() const override {
        return triangles.empty() ? mat() : triangles[0]->get_material();
    }

    void buildBVH() {
        if (!triangles.empty()) {
            std::vector<std::shared_ptr<hittable>> hittable_triangles;
            for (const auto& tri : triangles) {
                hittable_triangles.push_back(tri);
            }
            root_bvh = std::make_shared<BVHNode>(hittable_triangles, 0, hittable_triangles.size());
        }
        else {  // Add this else clause
            root_bvh = nullptr; // Ensure root_bvh is null if there are no triangles
        }
    }

    const std::vector<std::shared_ptr<triangle>>& getTriangles() const {
        return triangles;
    }

    bool is_point_inside(const point3& p) const override {

        for (const auto& tri : triangles)
        {
            if (tri->bounding_box().contains(p))
            {
                if (tri->is_point_inside(p))
                {
                    return true;
                }
            }

        }

        return false;
    }

    char test_bb(const BoundingBox& bb) const override {
        // Fast rejection: if the bounding box doesn't intersect the mesh's bounding box, return 'w'
        if (!bb.intersects(this->bounding_box())) {
            return 'w';
        }

        // Iterate through all triangles in the mesh
        bool all_corners_inside_any_triangle = true;
        bool any_corner_inside = false; // Flag for partial intersection

        for (const auto& tri : triangles) {
            char tri_result = tri->test_bb(bb);
            if (tri_result == 'g') {
                return 'g';  // Partial intersection: early exit
            }
            else if (tri_result == 'w') {
                all_corners_inside_any_triangle = false;
            }
        }
        // If all triangles are 'w' (outside), the result is 'w'
        if (!all_corners_inside_any_triangle) {
            return 'w';
        }

        //If after check all triangles and none is partial or outside, its full
        return 'b';
    }

    std::shared_ptr<hittable> clone() const override {
        auto newMesh = std::make_shared<Mesh>();

        // Deep copy all triangles (create new instances of each)
        for (const auto& tri : triangles) {
            newMesh->add_triangle(std::make_shared<triangle>(*tri)); // Create new triangle copies
        }

        newMesh->buildBVH(); // Rebuild BVH for the new mesh
        return newMesh;
    }

    std::shared_ptr<BVHNode> getBVH() const {
        return root_bvh;
    }

private:
    std::vector<std::shared_ptr<triangle>> triangles;
    std::shared_ptr<BVHNode> root_bvh = nullptr;

    bool defaultHitTraversal(const ray& r, interval ray_t, hit_record& rec) const {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& tri : triangles) {
            if (tri->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

struct MaterialData {
    color diffuse = color(1.0, 1.0, 1.0);
    double k_diffuse = 1.0;
    double k_specular = 0.5;
    double shininess = 50.0;
    double reflection = 0.0;
};

// ------------------------------------------------------------------
//                          OBJ File Loader
// ------------------------------------------------------------------

struct MeshData {
    std::vector<point3> vertices;
    std::vector<std::array<int, 3>> faces;
    std::vector<std::string> face_materials;
};

inline std::unordered_map<std::string, MaterialData> load_mtl(const std::string& filepath) {
    std::unordered_map<std::string, MaterialData> materials;
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Failed to open MTL file: " + filepath);

    std::string line, current_material;
    MaterialData mat = { color(1,1,1), 1.0, 0.5, 50.0, 0.0 }; // Default values

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            if (!current_material.empty()) materials[current_material] = mat;
            iss >> current_material;
            mat = { color(1,1,1), 1.0, 0.5, 50.0, 0.0 };
        }
        else if (prefix == "Kd") {
            double r, g, b; iss >> r >> g >> b;
            mat.diffuse = color(r, g, b);
        }
        else if (prefix == "Ns") {
            iss >> mat.shininess;
        }
        else if (prefix == "Ks") {
            double r, g, b; iss >> r >> g >> b;
            mat.k_specular = (r + g + b) / 2;
        }
    }
    if (!current_material.empty()) materials[current_material] = mat;
    return materials;
}

inline MeshData load_obj(const std::string& filepath, const std::unordered_map<std::string, MaterialData>& materials) {
    MeshData model;
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Failed to open OBJ file: " + filepath);

    std::string line, current_material;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            double x, y, z; iss >> x >> y >> z;
            model.vertices.emplace_back(x, y, z);
        }
        else if (prefix == "f") {
            std::vector<int> faceIndices; std::string vertexData;
            while (iss >> vertexData) {
                std::istringstream vertexStream(vertexData);
                int vertexIndex; vertexStream >> vertexIndex;
                faceIndices.push_back(vertexIndex - 1);
            }
            model.faces.push_back({ faceIndices[0], faceIndices[1], faceIndices[2] });
            model.face_materials.push_back(current_material);
        }
        else if (prefix == "usemtl") {
            iss >> current_material;
        }
    }
    return model;
}

inline ObjectID add_mesh_to_scene(const std::string& filepath, SceneManager& manager, const std::string& mtl_path = "", const mat& default_material = mat()) {
    std::unordered_map<std::string, MaterialData> materials;

    if (!mtl_path.empty()) {
        materials = load_mtl(mtl_path);
    }

    MeshData model = load_obj(filepath, materials);

    // Create a Mesh object
    auto mesh = std::make_shared<Mesh>();

    for (size_t i = 0; i < model.faces.size(); i++) {
        const auto& face = model.faces[i];
        mat material = default_material;

        if (!mtl_path.empty() && !model.face_materials[i].empty()) {
            const auto& mat_name = model.face_materials[i];
            if (materials.count(mat_name)) { // Check if material exists
                const auto& mat_data = materials.at(mat_name);
                material = mat(mat_data.diffuse, mat_data.k_diffuse, mat_data.k_specular, mat_data.shininess, 0.0);
            }
            else {
                std::cerr << "Warning: Material '" << mat_name << "' not found in MTL file." << std::endl;
            }

        }

        auto triangle_obj = std::make_shared<triangle>(
            model.vertices[face[0]], model.vertices[face[1]], model.vertices[face[2]], material
        );

        mesh->add_triangle(triangle_obj);
    }

    mesh->buildBVH();

    return manager.add(mesh);
}

#endif
