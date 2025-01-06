#ifndef MESH_H
#define MESH_H

#include <fstream>
#include <sstream>
#include <vector>
#include "vec3.h"
#include "triangle.h"
#include "hittable.h"
#include "hittable_list.h"

struct MeshOBJ {
    std::vector<point3> vertices;
    std::vector<std::array<int, 3>> faces;
};

MeshOBJ load_obj(const std::string& filepath) {
    MeshOBJ model;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + filepath);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        if (prefix == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            model.vertices.emplace_back(x, y, z);
        }
       else if (prefix == "f") {
            int v0, v1, v2;
            iss >> v0 >> v1 >> v2;
            // OBJ indices are 1-based; convert to 0-based
            model.faces.push_back({ v0 - 1, v1 - 1, v2 - 1 });
        }
    }

    return model;
}

void add_obj_to_scene(const MeshOBJ& model, hittable_list& scene, const mat& material) {
    for (const auto& face : model.faces) {
        scene.add(std::make_shared<triangle>(
            model.vertices[face[0]],
            model.vertices[face[1]],
            model.vertices[face[2]],
            material
        ));
    }
}

#endif