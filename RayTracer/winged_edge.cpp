#include "winged_edge.h"
#include "vec3.h"
#include "mesh.h"
#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <unordered_map>

//-------------------------------------------------------------------
// Face Methods
//-------------------------------------------------------------------

std::vector<Edge*> Face::getBoundary() const {
    return boundaryEdges;
}

std::vector<Vertex*> Face::getVertices() const {
    return vertices; // Directly return the stored order.
}

vec3 Face::normal() const {
    auto verts = getVertices();
    if (verts.size() < 3)
        return vec3(0, 0, 0);
    vec3 v0 = verts[0]->pos;
    vec3 v1 = verts[1]->pos;
    vec3 v2 = verts[2]->pos;
    return unit_vector(cross(v1 - v0, v2 - v0));
}



//-------------------------------------------------------------------
// WingedEdge Methods
//-------------------------------------------------------------------

// Find an existing edge between v1 and v2 or create a new one.
std::shared_ptr<Edge> WingedEdge::findOrCreateEdge(Vertex* v1, Vertex* v2) {
    EdgeKey key(v1->index, v2->index);
    auto it = edgeLookup.find(key);
    if (it != edgeLookup.end()) {
        return it->second;
    }
    auto newEdge = std::make_shared<Edge>(v1, v2);
    newEdge->index = static_cast<int>(edges.size());
    edges.push_back(newEdge);
    edgeLookup[key] = newEdge;

    // Set the incident edge pointers if not already assigned
    if (v1->incidentEdge == nullptr)
        v1->incidentEdge = newEdge.get();
    if (v2->incidentEdge == nullptr)
        v2->incidentEdge = newEdge.get();

    return newEdge;
}

// Create a face given an ordered boundary (list of vertices).
std::shared_ptr<Face> WingedEdge::createFace(const std::vector<Vertex*>& boundary) {
    if (boundary.size() < 3)
        throw std::invalid_argument("A face must have at least 3 vertices.");

    auto newFace = std::make_shared<Face>();
    newFace->index = static_cast<int>(faces.size());
    faces.push_back(newFace);

    // Store the ordered vertices directly.
    newFace->vertices = boundary;

    newFace->boundaryEdges.clear();

    size_t n = boundary.size();
    for (size_t i = 0; i < n; i++) {
        Vertex* v1 = boundary[i];
        Vertex* v2 = boundary[(i + 1) % n];
        auto edge = findOrCreateEdge(v1, v2);

        // Assign the face to the edge.
        if (edge->l_face.expired())
            edge->l_face = newFace;
        else if (edge->r_face.expired())
            edge->r_face = newFace;
        else
            throw std::runtime_error("Edge already has two adjacent faces.");

        newFace->boundaryEdges.push_back(edge.get());
    }

    // Set the starting edge for the face.
    if (!newFace->boundaryEdges.empty())
        newFace->edge = newFace->boundaryEdges[0];

    setupWingedEdgePointers();
    return newFace;
}

// Setup the wing pointers for all edges based on the face boundaries.
void WingedEdge::setupWingedEdgePointers() {
    // Reset all wing pointers.
    for (auto& e : edges) {
        e->ccw_l_edge = nullptr;
        e->cw_l_edge = nullptr;
        e->ccw_r_edge = nullptr;
        e->cw_r_edge = nullptr;
    }

    // For each face, traverse its boundary (using getBoundary) and set pointers.
    for (auto& f : faces) {
        auto boundary = f->getBoundary();
        size_t n = boundary.size();
        for (size_t i = 0; i < n; i++) {
            Edge* current = boundary[i];
            Edge* next = boundary[(i + 1) % n];

            // Determine orientation: if f is the left face of the current edge,
            // then assign next as the clockwise edge for the left side.
            if (current->l_face.lock().get() == f.get()) {
                current->cw_l_edge = next;
            }
            else if (current->r_face.lock().get() == f.get()) {
                current->cw_r_edge = next;
            }

            // For the previous edge pointer: find the previous edge in the boundary.
            Edge* prev = boundary[(i + n - 1) % n];
            if (current->l_face.lock().get() == f.get()) {
                current->ccw_l_edge = prev;
            }
            else if (current->r_face.lock().get() == f.get()) {
                current->ccw_r_edge = prev;
            }
        }
    }
}

// Print mesh information.
void WingedEdge::printInfo() {
    std::cout << "WingedEdge Mesh Information:" << std::endl;

    // Print vertices.
    std::cout << "Vertices: " << vertices.size() << std::endl;
    for (const auto& vertex : vertices) {
        std::cout << "  v" << vertex->index << ": ("
            << vertex->pos.x() << ", "
            << vertex->pos.y() << ", "
            << vertex->pos.z() << ")" << std::endl;
    }

    // Print edges.
    std::cout << "Edges: " << edges.size() << std::endl;
    for (const auto& e : edges) {
        std::cout << "  e" << e->index << ": ("
            << e->origin->pos.x() << ", " << e->origin->pos.y() << ", " << e->origin->pos.z()
            << ") -> ("
            << e->destination->pos.x() << ", " << e->destination->pos.y() << ", " << e->destination->pos.z()
            << ")" << std::endl;
    }

    // Print faces.
    std::cout << "Faces: " << faces.size() << std::endl;
    for (const auto& f : faces) {
        std::cout << "  f" << f->index << ": ";
        auto boundary = f->getBoundary();
        std::cout << "Boundary edges = ";
        for (Edge* e : boundary) {
            std::cout << "e" << e->index << " ";
        }
        vec3 n = f->normal();
        std::cout << " | Normal = (" << n.x() << ", " << n.y() << ", " << n.z() << ")" << std::endl;
    }
}

// Traverse the mesh by printing the vertices around each face.
void WingedEdge::traverseMesh() {
    std::cout << "\nMesh Traversal:" << std::endl;
    for (const auto& f : faces) {
        std::cout << "Face " << f->index << " vertices: ";
        auto verts = f->getVertices();  // Use the face's getVertices() method
        for (size_t i = 0; i < verts.size(); i++) {
            std::cout << "(" << verts[i]->pos.x() << ", " << verts[i]->pos.y() << ", " << verts[i]->pos.z() << ")";
            if (i < verts.size() - 1)
                std::cout << " -> ";
        }
        std::cout << std::endl;
    }
}


// Convert the WingedEdge mesh to a Mesh object.
std::shared_ptr<Mesh> WingedEdge::toMesh(const mat& material) const {
    auto mesh = std::make_shared<Mesh>();

    std::cout << "Converting WingedEdge to Mesh..." << std::endl;
    std::cout << "Total Faces: " << faces.size() << std::endl;

    for (const auto& face : faces) {
        auto verts = face->getVertices();
        std::cout << "Processing Face " << face->index << " with " << verts.size() << " vertices..." << std::endl;

        if (verts.size() == 3) {
            vec3 v0 = verts[0]->pos;
            vec3 v1 = verts[1]->pos;
            vec3 v2 = verts[2]->pos;

            std::cout << "  v0: (" << v0.x() << ", " << v0.y() << ", " << v0.z() << ")\n"
                << "  v1: (" << v1.x() << ", " << v1.y() << ", " << v1.z() << ")\n"
                << "  v2: (" << v2.x() << ", " << v2.y() << ", " << v2.z() << ")\n";

            auto tri = std::make_shared<triangle>(v0, v1, v2, material);
            mesh->add_triangle(tri);
        }
        else {
            std::cerr << "Warning: Face " << face->index << " is not a triangle ("
                << verts.size() << " vertices found). Skipping." << std::endl;
        }
    }

    std::cout << "Building BVH for the Mesh..." << std::endl;
    mesh->buildBVH();
    std::cout << "Conversion complete." << std::endl;

    return mesh;
}


//-------------------------------------------------------------------
// PrimitiveFactory Implementation
//-------------------------------------------------------------------
std::unique_ptr<WingedEdge> PrimitiveFactory::createTetrahedron() {
    auto mesh = std::make_unique<WingedEdge>();

    vec3 v0(0.0, 0.0, 0.0);
    vec3 v1(1.0, 0.0, 0.0);
    vec3 v2(0.5, 0.0, 0.866025);
    vec3 v3(0.5, 0.816496, 0.288675);

    mesh->vertices.push_back(std::make_unique<Vertex>(v0, 0));
    mesh->vertices.push_back(std::make_unique<Vertex>(v1, 1));
    mesh->vertices.push_back(std::make_unique<Vertex>(v2, 2));
    mesh->vertices.push_back(std::make_unique<Vertex>(v3, 3));

    // Create faces using the vertices.
    // Base face (flat on the XZ plane):
    mesh->createFace({ mesh->vertices[0].get(), mesh->vertices[1].get(), mesh->vertices[2].get() });
    // Side faces:
    mesh->createFace({ mesh->vertices[0].get(), mesh->vertices[3].get(), mesh->vertices[1].get() });
    mesh->createFace({ mesh->vertices[1].get(), mesh->vertices[3].get(), mesh->vertices[2].get() });
    mesh->createFace({ mesh->vertices[2].get(), mesh->vertices[3].get(), mesh->vertices[0].get() });

    mesh->setupWingedEdgePointers();
    return mesh;
}

//-------------------------------------------------------------------
// MeshCollection Implementation
//-------------------------------------------------------------------
void MeshCollection::addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name) {
    if (!name.empty()) {
        if (nameToIndexMap_.count(name) > 0)
            throw std::invalid_argument("Mesh with name '" + name + "' already exists.");
        nameToIndexMap_[name] = meshes_.size();
    }
    meshes_.push_back(std::move(mesh));
}

void MeshCollection::removeMesh(size_t index) {
    if (index >= meshes_.size())
        throw std::out_of_range("Mesh index out of range.");

    // Remove the name from the map if it exists.
    for (auto it = nameToIndexMap_.begin(); it != nameToIndexMap_.end(); ++it) {
        if (it->second == index) {
            nameToIndexMap_.erase(it);
            break;
        }
        else if (it->second > index) {
            it->second--;
        }
    }
    meshes_.erase(meshes_.begin() + index);
}

const WingedEdge* MeshCollection::getMesh(size_t index) const {
    if (index >= meshes_.size())
        throw std::out_of_range("Mesh index out of range.");
    return meshes_[index].get();
}

WingedEdge* MeshCollection::getMesh(size_t index) {
    if (index >= meshes_.size())
        throw std::out_of_range("Mesh index out of range.");
    return meshes_[index].get();
}

const WingedEdge* MeshCollection::getMeshByName(const std::string& name) const {
    auto it = nameToIndexMap_.find(name);
    if (it != nameToIndexMap_.end())
        return meshes_[it->second].get();
    return nullptr;
}

WingedEdge* MeshCollection::getMeshByName(const std::string& name) {
    auto it = nameToIndexMap_.find(name);
    if (it != nameToIndexMap_.end())
        return meshes_[it->second].get();
    return nullptr;
}

size_t MeshCollection::getMeshCount() const {
    return meshes_.size();
}

void MeshCollection::clear() {
    meshes_.clear();
    nameToIndexMap_.clear();
}

void MeshCollection::printInfo() const {
    std::cout << "MeshCollection contains " << meshes_.size() << " mesh(es)." << std::endl;
    for (size_t i = 0; i < meshes_.size(); i++) {
        std::cout << "\nMesh " << i << ":" << std::endl;
        std::string meshName = "";
        for (const auto& pair : nameToIndexMap_) {
            if (pair.second == i) {
                meshName = pair.first;
                break;
            }
        }
        if (!meshName.empty())
            std::cout << "  Name: " << meshName << std::endl;
        meshes_[i]->printInfo();
    }
}

void MeshCollection::traverseMeshes() const {
    for (size_t i = 0; i < meshes_.size(); i++) {
        std::cout << "\nTraversing Mesh " << i << ":" << std::endl;
        meshes_[i]->traverseMesh();
    }
}

std::string MeshCollection::getMeshName(const WingedEdge* mesh) const {
    for (const auto& pair : nameToIndexMap_) {
        if (meshes_[pair.second].get() == mesh) {
            return pair.first;  // Return the associated name if found.
        }
    }
    return "";  // Return an empty string if the mesh is unnamed.
}