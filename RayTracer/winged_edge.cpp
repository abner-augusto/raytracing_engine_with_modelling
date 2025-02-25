#include "winged_edge.h"
#include <iostream>
#include <stdexcept>

// Helper function to compare vec3 objects lexicographically.
bool compareVec3(const vec3& a, const vec3& b) {
    if (a.x() != b.x()) return a.x() < b.x();
    if (a.y() != b.y()) return a.y() < b.y();
    return a.z() < b.z();
}

// EdgeKey implementation
EdgeKey::EdgeKey(const vec3& a, const vec3& b) {
    if (a == b) {
        throw std::invalid_argument("Edge cannot have identical vertices.");
    }
    if (compareVec3(a, b)) {
        v1 = a;
        v2 = b;
    }
    else {
        v1 = b;
        v2 = a;
    }
}

bool EdgeKey::operator==(const EdgeKey& other) const {
    return (v1 == other.v1 && v2 == other.v2);
}


// std::hash specializations implementation
namespace std {
    size_t hash<vec3>::operator()(const vec3& v) const {
        std::hash<double> hasher;
        size_t h1 = hasher(v.x());
        size_t h2 = hasher(v.y());
        size_t h3 = hasher(v.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }

    size_t hash<EdgeKey>::operator()(const EdgeKey& key) const {
        size_t h1 = std::hash<vec3>{}(key.v1);
        size_t h2 = std::hash<vec3>{}(key.v2);
        return h1 ^ (h2 << 1);
    }
}

// edge implementation
edge::edge(const vec3& orig, const vec3& dest)
    : origVec(orig), destVec(dest) {
}

bool edge::operator==(const edge& other) const {
    return ((origVec == other.origVec && destVec == other.destVec) ||
        (origVec == other.destVec && destVec == other.origVec));
}

// face implementation
vec3 face::normal() const {
    if (vEdges.size() < 3) return vec3(0, 0, 0);

    edge* e0 = vEdges[0];
    edge* e1 = vEdges[1];

    vec3 v0 = (vIsReversed[0] ? e0->origVec - e0->destVec : e0->destVec - e0->origVec);
    vec3 v1 = (vIsReversed[1] ? e1->origVec - e1->destVec : e1->destVec - e1->origVec);

    return unit_vector(cross(v0, v1));
}

// WingedEdge implementation
WingedEdge::WingedEdge() = default;
WingedEdge::~WingedEdge() = default;

std::shared_ptr<edge> WingedEdge::findOrCreateEdge(const vec3& v1, const vec3& v2) {
    EdgeKey key(v1, v2);
    auto it = edgeLookup.find(key);
    if (it != edgeLookup.end()) {
        return it->second;
    }
    auto newEdge = std::make_shared<edge>(v1, v2);
    newEdge->index = edges.size();
    edges.push_back(newEdge);
    edgeLookup[key] = newEdge;
    return newEdge;
}

std::shared_ptr<face> WingedEdge::createTriangularFace(const vec3& v1, const vec3& v2, const vec3& v3) {
    auto newFace = std::make_shared<face>();
    newFace->index = faces.size();
    faces.push_back(newFace);

    auto e1 = findOrCreateEdge(v1, v2);
    auto e2 = findOrCreateEdge(v2, v3);
    auto e3 = findOrCreateEdge(v3, v1);

    newFace->vEdges = { e1.get(), e2.get(), e3.get() };
    newFace->vIsReversed = {
        !(e1->origVec == v1 && e1->destVec == v2),
        !(e2->origVec == v2 && e2->destVec == v3),
        !(e3->origVec == v3 && e3->destVec == v1)
    };

    // Update face references in edges. Use weak_ptr assignment.
    if (e1->l_face.expired()) e1->l_face = newFace;
    else if (e1->r_face.expired()) e1->r_face = newFace;

    if (e2->l_face.expired()) e2->l_face = newFace;
    else if (e2->r_face.expired()) e2->r_face = newFace;

    if (e3->l_face.expired()) e3->l_face = newFace;
    else if (e3->r_face.expired()) e3->r_face = newFace;

    // Recompute wing pointers for consistency.
    setupWingedEdgePointers();
    return newFace;
}

void WingedEdge::setupWingedEdgePointers() {
    // Reset all wing pointers.
    for (auto& e : edges) {
        e->ccw_l_edge = nullptr;
        e->ccw_r_edge = nullptr;
        e->cw_l_edge = nullptr;
        e->cw_r_edge = nullptr;
    }
    // For each face, update pointers based on the edge order.
    for (auto& f : faces) {
        for (size_t i = 0; i < f->vEdges.size(); i++) {
            edge* current = f->vEdges[i];
            edge* next = f->vEdges[(i + 1) % f->vEdges.size()];
            bool currentReversed = f->vIsReversed[i];
            bool nextReversed = f->vIsReversed[(i + 1) % f->vEdges.size()];

            // Assign pointers based on whether f is the left or right face of the current edge.
            if (current->l_face.lock().get() == f.get()) {
                if (currentReversed)
                    current->ccw_l_edge = next;
                else
                    current->cw_l_edge = next;
            }
            else if (current->r_face.lock().get() == f.get()) {
                if (currentReversed)
                    current->ccw_r_edge = next;
                else
                    current->cw_r_edge = next;
            }

            // For the next edge, assign pointers in the reverse direction.
            if (next->l_face.lock().get() == f.get()) {
                if (nextReversed)
                    next->cw_l_edge = current;
                else
                    next->ccw_l_edge = current;
            }
            else if (next->r_face.lock().get() == f.get()) {
                if (nextReversed)
                    next->cw_r_edge = current;
                else
                    next->ccw_r_edge = current;
            }
        }
    }
}

void WingedEdge::printInfo() {
    std::cout << "WingedEdge Mesh Information:" << std::endl;
    std::cout << "Vertices: " << vertices.size() << std::endl;
    for (size_t i = 0; i < vertices.size(); i++) {
        std::cout << "  v" << i << ": ("
            << vertices[i].x() << ", "
            << vertices[i].y() << ", "
            << vertices[i].z() << ")" << std::endl;
    }

    std::cout << "Edges: " << edges.size() << std::endl;
    for (auto& e : edges) {
        std::cout << "  e" << e->index << ": ("
            << e->origVec.x() << "," << e->origVec.y() << "," << e->origVec.z() << ") -> ("
            << e->destVec.x() << "," << e->destVec.y() << "," << e->destVec.z() << ")" << std::endl;
    }

    std::cout << "Faces: " << faces.size() << std::endl;
    for (auto& f : faces) {
        std::cout << "  f" << f->index << ": Edges = ";
        for (size_t i = 0; i < f->vEdges.size(); i++) {
            std::cout << "e" << f->vEdges[i]->index;
            if (f->vIsReversed[i]) std::cout << "(R)";
            if (i < f->vEdges.size() - 1) std::cout << ", ";
        }
        vec3 n = f->normal();
        std::cout << " | Normal = (" << n.x() << ", " << n.y() << ", " << n.z() << ")" << std::endl;
    }
}

void WingedEdge::traverseMesh() {
    std::cout << "\nMesh Traversal:" << std::endl;
    for (auto& f : faces) {
        std::cout << "Face " << f->index << " vertices: ";
        edge* startEdge = f->vEdges[0];
        edge* currentEdge = startEdge;
        bool isReversed = f->vIsReversed[0];

        std::cout << "(" << (isReversed ? currentEdge->destVec : currentEdge->origVec) << ") -> ";
        do {
            std::cout << "(" << (isReversed ? currentEdge->origVec : currentEdge->destVec) << ")";
            edge* nextEdge = nullptr;
            if (currentEdge->l_face.lock().get() == f.get())
                nextEdge = isReversed ? currentEdge->ccw_l_edge : currentEdge->cw_l_edge;
            else
                nextEdge = isReversed ? currentEdge->ccw_r_edge : currentEdge->cw_r_edge;

            currentEdge = nextEdge;
            for (size_t i = 0; i < f->vEdges.size(); i++) {
                if (f->vEdges[i] == currentEdge) {
                    isReversed = f->vIsReversed[i];
                    break;
                }
            }
            if (currentEdge != startEdge) std::cout << " -> ";
        } while (currentEdge != startEdge);
        std::cout << std::endl;
    }
}

std::unique_ptr<Mesh> WingedEdge::toMesh(const mat& material) const {
    auto mesh = std::make_unique<Mesh>();

    // Iterate through each face in the WingedEdge structure.
    for (const auto& face : faces) {
        // Ensure the face is a triangle (has 3 edges).
        if (face->vEdges.size() == 3) {
            // Get the three vertices of the triangle.  Handle the
            // potential reversal of edges.
            vec3 v0 = face->vIsReversed[0] ? face->vEdges[0]->destVec : face->vEdges[0]->origVec;
            vec3 v1 = face->vIsReversed[1] ? face->vEdges[1]->destVec : face->vEdges[1]->origVec;
            vec3 v2 = face->vIsReversed[2] ? face->vEdges[2]->destVec : face->vEdges[2]->origVec;

            // Create a triangle object and add it to the Mesh.
            auto tri = std::make_shared<triangle>(v0, v1, v2, material);
            mesh->add_triangle(tri);
        }
        else {
            //for not triangle faces
        }
    }
    mesh->buildBVH();
    return mesh;
}

// PrimitiveFactory implementation
std::unique_ptr<WingedEdge> PrimitiveFactory::createTetrahedron() {
    auto mesh = std::make_unique<WingedEdge>();
    vec3 v0(0.0, 0.0, 0.0);
    vec3 v1(1.0, 0.0, 0.0);
    vec3 v2(0.5, 0.866, 0.0);
    vec3 v3(0.5, 0.289, 0.816);
    mesh->vertices = { v0, v1, v2, v3 };

    mesh->createTriangularFace(v0, v1, v2);
    mesh->createTriangularFace(v0, v3, v1);
    mesh->createTriangularFace(v1, v3, v2);
    mesh->createTriangularFace(v2, v3, v0);


    for (auto& f : mesh->faces) {
        f->bIsTetra = true;
    }
    mesh->setupWingedEdgePointers();
    return mesh;
}

// Scene implementation
void MeshCollection::addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name) {
    if (!name.empty()) {
        if (nameToIndexMap_.count(name) > 0) {
            throw std::invalid_argument("Mesh with name '" + name + "' already exists.");
        }
        nameToIndexMap_[name] = meshes_.size();
    }
    meshes_.push_back(std::move(mesh));
}

void MeshCollection::removeMesh(size_t index) {
    if (index >= meshes_.size()) {
        throw std::out_of_range("Mesh index out of range.");
    }

    // Remove the name from the map if it exists.
    for (auto it = nameToIndexMap_.begin(); it != nameToIndexMap_.end(); ++it) {
        if (it->second == index) {
            nameToIndexMap_.erase(it);
            break;
        }
        else if (it->second > index) {
            // Decrement indices of subsequent meshes in the map.
            it->second--;
        }
    }

    meshes_.erase(meshes_.begin() + index);
}


const WingedEdge* MeshCollection::getMesh(size_t index) const {
    if (index >= meshes_.size()) {
        throw std::out_of_range("Mesh index out of range.");
    }
    return meshes_[index].get();
}

WingedEdge* MeshCollection::getMesh(size_t index) {
    if (index >= meshes_.size()) {
        throw std::out_of_range("Mesh index out of range.");
    }
    return meshes_[index].get();
}

const WingedEdge* MeshCollection::getMeshByName(const std::string& name) const {
    auto it = nameToIndexMap_.find(name);
    if (it != nameToIndexMap_.end()) {
        return meshes_[it->second].get();
    }
    return nullptr;
}
WingedEdge* MeshCollection::getMeshByName(const std::string& name) {
    auto it = nameToIndexMap_.find(name);
    if (it != nameToIndexMap_.end()) {
        return meshes_[it->second].get();
    }
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
        // Check if the mesh has a name.
        std::string meshName = "";
        for (const auto& pair : nameToIndexMap_) {
            if (pair.second == i) {
                meshName = pair.first;
                break;
            }
        }
        if (!meshName.empty()) {
            std::cout << "  Name: " << meshName << std::endl;
        }
        meshes_[i]->printInfo();
    }
}

void MeshCollection::traverseMeshes() const {
    for (size_t i = 0; i < meshes_.size(); i++) {
        std::cout << "\nTraversing Mesh " << i << ":" << std::endl;
        meshes_[i]->traverseMesh();
    }
}