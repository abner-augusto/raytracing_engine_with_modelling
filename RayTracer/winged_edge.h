#ifndef WINGED_EDGE_H
#define WINGED_EDGE_H

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <functional>
#include "vec3.h"
#include "mesh.h"

// Forward declarations
struct edge;
struct face;
class WingedEdge;


// EdgeKey structure
struct EdgeKey {
    vec3 v1, v2;

    EdgeKey(const vec3& a, const vec3& b);

    bool operator==(const EdgeKey& other) const;
};

// std::hash specializations
namespace std {
    template <>
    struct hash<vec3> {
        size_t operator()(const vec3& v) const;
    };

    template <>
    struct hash<EdgeKey> {
        size_t operator()(const EdgeKey& key) const;
    };
}


struct edge {
    vec3 origVec;
    vec3 destVec;

    // Wing pointers
    edge* ccw_l_edge = nullptr;
    edge* ccw_r_edge = nullptr;
    edge* cw_l_edge = nullptr;
    edge* cw_r_edge = nullptr;

    // Weak pointers to adjacent faces
    std::weak_ptr<face> l_face;
    std::weak_ptr<face> r_face;
    int index = -1;

    edge(const vec3& orig, const vec3& dest);

    bool operator==(const edge& other) const;
};

struct face {
    int index = -1;
    std::vector<edge*> vEdges;
    std::vector<bool> vIsReversed;
    bool bIsTetra = false;

    vec3 normal() const;
};



class WingedEdge {
public:
    std::vector<vec3> vertices;
    std::vector<std::shared_ptr<edge>> edges;
    std::vector<std::shared_ptr<face>> faces;
    std::unordered_map<EdgeKey, std::shared_ptr<edge>> edgeLookup;

    WingedEdge();
    ~WingedEdge();

    std::shared_ptr<edge> findOrCreateEdge(const vec3& v1, const vec3& v2);
    std::shared_ptr<face> createTriangularFace(const vec3& v1, const vec3& v2, const vec3& v3);
    void setupWingedEdgePointers();
    void printInfo();
    void traverseMesh();

    std::unique_ptr<Mesh> toMesh(const mat& material = mat()) const;
};

// Factory class
class PrimitiveFactory {
public:
    static std::unique_ptr<WingedEdge> createTetrahedron();
    // Add other primitive creation methods here as needed.
};

// Scene class
class MeshCollection {
public:
    // Add a mesh to the collection with an optional name.
    void addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name = "");

    // Remove a mesh by its index.  Throws if index is out of range.
    void removeMesh(size_t index);

    // Get a mesh by its index (const access).  Throws if out of range.
    const WingedEdge* getMesh(size_t index) const;

    // Get a mesh by its index (mutable access). Throws if out of range.
    WingedEdge* getMesh(size_t index);


    // Get a mesh by its name (const access). Returns nullptr if not found.
    const WingedEdge* getMeshByName(const std::string& name) const;

    // Get a mesh by its name (mutable access). Returns nullptr if not found.
    WingedEdge* getMeshByName(const std::string& name);

    // Get the number of meshes in the collection.
    size_t getMeshCount() const;

    // Clear all meshes from the collection.
    void clear();

    // Print information about all meshes in the collection.
    void printInfo() const;

    // Traverse all meshes in the collection.
    void traverseMeshes() const;


    // --- Iterators (added for easier access to meshes) ---

    // Define iterator types using the underlying vector's iterators.
    using iterator = typename std::vector<std::unique_ptr<WingedEdge>>::iterator;
    using const_iterator = typename std::vector<std::unique_ptr<WingedEdge>>::const_iterator;

    // Begin and end iterators (for range-based for loops and standard algorithms).
    iterator begin() { return meshes_.begin(); }
    iterator end() { return meshes_.end(); }
    const_iterator begin() const { return meshes_.begin(); }
    const_iterator end() const { return meshes_.end(); }
    const_iterator cbegin() const { return meshes_.cbegin(); } // Added const begin/end
    const_iterator cend() const { return meshes_.cend(); }

private:
    std::vector<std::unique_ptr<WingedEdge>> meshes_;
    std::unordered_map<std::string, size_t> nameToIndexMap_; // Map mesh names to indices.
};

// Helper function (declared here, defined in .cpp)
bool compareVec3(const vec3& a, const vec3& b);


#endif // WINGED_EDGE_H