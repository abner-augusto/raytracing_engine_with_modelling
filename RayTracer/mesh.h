#ifndef MESH_H
#define MESH_H

#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include "vec3.h"
#include "triangle.h"
#include "hittable_manager.h"
#include "material.h"

struct Material {
    color diffuse = color(1.0, 1.0, 1.0);
    double k_diffuse = 0.8;
    double k_specular = 0.3;
    double shininess = 10.0;
    double reflection = 0.0;
};

struct MeshOBJ {
    std::vector<point3> vertices;
    std::vector<std::array<int, 3>> faces;
    std::vector<std::string> face_materials;
    std::vector<ObjectID> triangle_ids;
    std::optional<point3> first_vertex;
};

std::unordered_map<std::string, Material> load_mtl(const std::string& filepath) {
    std::unordered_map<std::string, Material> materials;
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Failed to open MTL file: " + filepath);

    std::string line, current_material;
    Material mat = { color(1,1,1), 0.8, 0.3, 10.0, 0.0 }; // Default values

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            if (!current_material.empty()) materials[current_material] = mat;
            iss >> current_material;
            mat = { color(1,1,1), 0.8, 0.3, 10.0, 0.0 };
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

MeshOBJ load_obj(const std::string& filepath, const std::unordered_map<std::string, Material>& materials) {
    MeshOBJ model;
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
            if (!model.first_vertex) model.first_vertex = point3(x, y, z);
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


MeshOBJ add_mesh_to_manager(const std::string& filepath, HittableManager& manager, const std::string& mtl_path = "", const mat& default_material = mat()) {
    std::unordered_map<std::string, Material> materials;

    if (!mtl_path.empty()) {
        materials = load_mtl(mtl_path);
    }

    MeshOBJ model = load_obj(filepath, materials);

    for (size_t i = 0; i < model.faces.size(); i++) {
        const auto& face = model.faces[i];
        mat material = default_material;

        if (!mtl_path.empty() && !model.face_materials[i].empty()) {
            const auto& mat_name = model.face_materials[i];
            const auto& mat_data = materials.at(mat_name);
            material = mat(mat_data.diffuse, mat_data.k_diffuse, mat_data.k_specular, mat_data.shininess, 0.0);
        }

        auto triangle_obj = std::make_shared<triangle>(
            model.vertices[face[0]], model.vertices[face[1]], model.vertices[face[2]], material
        );

        model.triangle_ids.push_back(manager.add(triangle_obj));
    }
    return model;
}


#endif
