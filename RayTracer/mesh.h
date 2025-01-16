#ifndef MESH_H
#define MESH_H

#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include "vec3.h"
#include "triangle.h"
#include "hittable_manager.h"

struct MeshOBJ {
    std::vector<point3> vertices;          // Stores all vertices
    std::vector<std::array<int, 3>> faces; // Stores all faces
    std::vector<ObjectID> triangle_ids;    // Stores triangle IDs
    std::optional<point3> first_vertex;    // Stores the first vertex (optional to handle empty files)
};


MeshOBJ load_obj(const std::string& filepath) {
    MeshOBJ model;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + filepath);
    }

    std::string line;

    while (std::getline(file, line)) {
        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Parse vertex line
            double x, y, z;
            iss >> x >> y >> z;
            if (iss.fail()) {
                throw std::runtime_error("Invalid vertex line in OBJ file: " + line);
            }
            point3 vertex(x, y, z);
            model.vertices.push_back(vertex);

            // Store the first vertex if it's not already set
            if (!model.first_vertex.has_value()) {
                model.first_vertex = vertex;
            }
        }
        else if (prefix == "f") {
            // Parse face line
            std::vector<int> faceIndices;
            std::string vertexData;

            while (iss >> vertexData) {
                std::istringstream vertexStream(vertexData);
                int vertexIndex;
                char separator;

                // Check for '/' to determine if it's complex
                if (vertexData.find('/') != std::string::npos) {
                    vertexStream >> vertexIndex >> separator;
                    vertexStream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
                }
                else {
                    vertexStream >> vertexIndex;
                }

                if (vertexStream.fail()) {
                    throw std::runtime_error("Invalid face line in OBJ file: " + line);
                }
                faceIndices.push_back(vertexIndex - 1); // Convert to 0-based index
            }

            if (faceIndices.size() != 3) {
                throw std::runtime_error("Only triangular faces are supported: " + line);
            }

            model.faces.push_back({ faceIndices[0], faceIndices[1], faceIndices[2] });
        }
    }

    file.close();

    std::cout << "OBJ file loaded with " << model.vertices.size() << " vertices and "
        << model.faces.size() << " faces from " << filepath << "\n";


    return model;
}




MeshOBJ add_mesh_to_manager(
    const std::string& filepath,
    HittableManager& manager,
    const mat& material
) {
    // Load the OBJ model
    MeshOBJ model = load_obj(filepath);

    // Create and add triangles to the manager, storing their IDs
    for (const auto& face : model.faces) {
        auto triangle_obj = std::make_shared<triangle>(
            model.vertices[face[0]],
            model.vertices[face[1]],
            model.vertices[face[2]],
            material
        );

        ObjectID id = manager.add(triangle_obj);
        model.triangle_ids.push_back(id);
        //std::cout << "Added triangle with ID: " << id << "\n";
    }

    std::cout << "Added mesh to manager with " << model.triangle_ids.size() << " triangles.\n";

    return model; // Return the updated MeshOBJ
}

#endif
