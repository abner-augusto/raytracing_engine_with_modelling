#ifndef WINGED_EDGE_H
#define WINGED_EDGE_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "vec3.h"
#include "mesh.h"

// Forward declarations
struct Edge;
struct Face;
struct Vertex;
class WingedEdge;

//-------------------------------------------------------------------
// Vertex Table
//-------------------------------------------------------------------
// Each vertex stores its coordinates and a pointer to one incident edge.
struct Vertex {
    vec3 pos;
    int index;
    Edge* incidentEdge;  // Pointer to one incident edge

    Vertex(const vec3& position, int idx)
        : pos(position), index(idx), incidentEdge(nullptr) {
    }
};

//-------------------------------------------------------------------
// Edge Table
//-------------------------------------------------------------------
// Each edge stores pointers to its two end vertices, pointers to its
// adjacent faces (left and right), and four wing pointer.
struct Edge {
    Vertex* origin;
    Vertex* destination;

    // Wing pointers
    Edge* ccw_l_edge = nullptr; // Counterclockwise next edge on left face
    Edge* cw_l_edge = nullptr; // Clockwise previous edge on left face
    Edge* ccw_r_edge = nullptr; // Counterclockwise next edge on right face
    Edge* cw_r_edge = nullptr; // Clockwise previous edge on right face

    // Adjacent face pointers (using weak_ptr to avoid ownership cycles)
    std::weak_ptr<Face> l_face;
    std::weak_ptr<Face> r_face;

    int index = -1;

    Edge(Vertex* orig, Vertex* dest)
        : origin(orig), destination(dest) {
    }

    // Equality of an edge is determined by its vertices (order-independent)
    bool operator==(const Edge& other) const {
        return ((origin == other.origin && destination == other.destination) ||
            (origin == other.destination && destination == other.origin));
    }
};

// A key to look up an edge uniquely (using vertex indices)
struct EdgeKey {
    int v1, v2; // stored so that v1 < v2

    EdgeKey(int a, int b) {
        if (a == b)
            throw std::invalid_argument("Edge cannot have identical vertices.");
        if (a < b) { v1 = a; v2 = b; }
        else { v1 = b; v2 = a; }
    }

    bool operator==(const EdgeKey& other) const {
        return v1 == other.v1 && v2 == other.v2;
    }
};

// Specialize std::hash for EdgeKey so that it can be used in unordered_map.
namespace std {
    template <>
    struct hash<EdgeKey> {
        size_t operator()(const EdgeKey& key) const {
            size_t h1 = std::hash<int>{}(key.v1);
            size_t h2 = std::hash<int>{}(key.v2);
            return h1 ^ (h2 << 1);
        }
    };
}

//-------------------------------------------------------------------
// Face Table
//-------------------------------------------------------------------
// A face is represented by storing a pointer to one of its boundary edges.
// The complete boundary can be obtained by following the wing pointers.
struct Face {
    int index = -1;
    Edge* edge = nullptr;              // Pointer to one of the boundary edges
    std::vector<Edge*> boundaryEdges;  // The boundary edges (if needed)
    std::vector<Vertex*> vertices;     // The ordered vertices of the face

    Face() : edge(nullptr) {}

    // Return the stored ordered vertices.
    std::vector<Vertex*> getVertices() const;
    std::vector<Edge*> getBoundary() const;

    vec3 normal() const;
};


//-------------------------------------------------------------------
// WingedEdge Mesh Structure
//-------------------------------------------------------------------
class WingedEdge {
public:
    std::vector<std::unique_ptr<Vertex>> vertices;
    std::vector<std::shared_ptr<Edge>> edges;
    std::vector<std::shared_ptr<Face>> faces;

    // Lookup table for edges based on their end vertices
    std::unordered_map<EdgeKey, std::shared_ptr<Edge>> edgeLookup;

    WingedEdge() = default;
    ~WingedEdge() = default;

    // Find an existing edge or create a new one between two vertices.
    std::shared_ptr<Edge> findOrCreateEdge(Vertex* v1, Vertex* v2);

    // Create a face given an ordered boundary (a list of vertices).
    std::shared_ptr<Face> createFace(const std::vector<Vertex*>& boundary);

    // Setup the wing pointers for all edges based on the face boundaries.
    void setupWingedEdgePointers();

    // Utility functions to print and traverse the mesh.
    void printInfo();
    void traverseMesh();

    std::shared_ptr<Mesh> toMesh(const mat& material = mat()) const;
};

//-------------------------------------------------------------------
// Primitive Factory and Mesh Collection (scene management)
//-------------------------------------------------------------------
class PrimitiveFactory {
public:
    static std::unique_ptr<WingedEdge> createTetrahedron();
    // Additional primitive creation methods can be added.
};

class MeshCollection {
public:
    // Add, remove, access meshes as needed...
    void addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name = "");
    void removeMesh(size_t index);
    const WingedEdge* getMesh(size_t index) const;
    WingedEdge* getMesh(size_t index);
    const WingedEdge* getMeshByName(const std::string& name) const;
    std::string getMeshName(const WingedEdge* mesh) const;
    WingedEdge* getMeshByName(const std::string& name);
    size_t getMeshCount() const;
    void clear();
    void printInfo() const;
    void traverseMeshes() const;

private:
    std::vector<std::unique_ptr<WingedEdge>> meshes_;
    std::unordered_map<std::string, size_t> nameToIndexMap_;
};

#endif // WINGED_EDGE_H