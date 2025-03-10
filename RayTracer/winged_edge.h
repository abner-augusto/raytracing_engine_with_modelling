#ifndef WINGED_EDGE_H
#define WINGED_EDGE_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <string>
#include "vec3.h"
#include "mesh.h"

// Forward declarations
struct Edge;
struct Face;
struct Vertex;
class WingedEdge;

/**
 * @brief Represents a vertex in the Winged Edge data structure.
 */
struct Vertex {
    vec3 pos;               ///< Position of the vertex.
    int index;              ///< Index of the vertex.
    Edge* incidentEdge;     ///< Pointer to one incident edge.

    Vertex(const vec3& position, int idx)
        : pos(position), index(idx), incidentEdge(nullptr) {
    }
};

/**
 * @brief Represents an edge in the Winged Edge data structure.
 */
struct Edge {
    Vertex* origin;         ///< Pointer to the origin vertex.
    Vertex* destination;    ///< Pointer to the destination vertex.

    Edge* counterClockwiseLeftEdge = nullptr;   ///< Counterclockwise next edge on left face.
    Edge* clockwiseLeftEdge = nullptr;            ///< Clockwise previous edge on left face.
    Edge* counterClockwiseRightEdge = nullptr;    ///< Counterclockwise next edge on right face.
    Edge* clockwiseRightEdge = nullptr;           ///< Clockwise previous edge on right face.

    // Adjacent face pointers (using weak_ptr to avoid ownership cycles)
    std::weak_ptr<Face> leftFace;
    std::weak_ptr<Face> rightFace;

    int index = -1;

    Edge(Vertex* originVertex, Vertex* destinationVertex)
        : origin(originVertex), destination(destinationVertex) {
    }

    /**
     * @brief Equality operator for edges (order-independent).
     */
    bool operator==(const Edge& other) const {
        return ((origin == other.origin && destination == other.destination) ||
            (origin == other.destination && destination == other.origin));
    }
};

/**
 * @brief Unique key for an edge based on vertex indices.
 */
struct EdgeKey {
    int v1, v2; ///< Vertex indices (stored so that v1 < v2)

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

/**
 * @brief Represents a face in the Winged Edge data structure.
 */
struct Face {
    int index = -1;                     ///< Index of the face.
    Edge* edge = nullptr;               ///< Pointer to one of the boundary edges.
    std::vector<Edge*> boundaryEdges;   ///< The boundary edges.
    std::vector<Vertex*> vertices;      ///< The ordered vertices of the face.

    mutable bool normalCached = false;  ///< Flag indicating if the normal is cached.
    mutable vec3 cachedNormal;          ///< Cached normal vector.

    Face() : edge(nullptr) {}

    /**
     * @brief Invalidates the cached normal vector.
     *
     * Call this method whenever the vertices of the face are modified to ensure
     * the normal is recomputed on the next call to normal().
     */
    inline void invalidateCache() { normalCached = false; }

    /**
     * @brief Computes and returns the normal of the face.
     * Caches the result for subsequent calls.
     * @return The unit normal vector.
     */
    vec3 normal() const;
};


/**
 * @brief Winged Edge Mesh Structure.
 */
class WingedEdge {
public:
    std::vector<std::unique_ptr<Vertex>> vertices;                    ///< List of vertices.
    std::vector<std::shared_ptr<Edge>> edges;                           ///< List of edges.
    std::vector<std::shared_ptr<Face>> faces;                           ///< List of faces.

    /// Lookup table for edges based on their end vertices.
    std::unordered_map<EdgeKey, std::shared_ptr<Edge>> edgeLookup;

    WingedEdge() = default;
    ~WingedEdge() = default;

    /**
     * @brief Finds an existing edge between two vertices or creates a new one.
     * @param vertex1 Pointer to the first vertex.
     * @param vertex2 Pointer to the second vertex.
     * @return Shared pointer to the edge.
     */
    std::shared_ptr<Edge> findOrCreateEdge(Vertex* vertex1, Vertex* vertex2);

    /**
     * @brief Creates a face given an ordered boundary (list of vertices).
     * @param boundary The list of vertices forming the face boundary.
     * @return Shared pointer to the created face.
     */
    std::shared_ptr<Face> createFace(const std::vector<Vertex*>& boundary);

    /**
     * @brief Sets up the wing pointers for all edges based on the face boundaries.
     * @brief Clears all pointers and redo all wing pointers in the vertex, edge and face.
     */
    void setupWingedEdgePointers();

    /**
     * @brief Prints mesh information.
     */
    void printInfo() const;

    /**
     * @brief Traverses the mesh and prints vertices around each face.
     */
    void traverseMesh() const;

    /**
     * @brief Converts the WingedEdge mesh to a renderable Mesh object.
     * @param material Material used for the mesh.
     * @return Shared pointer to the Mesh object.
     */
    std::shared_ptr<Mesh> toMesh(const mat& material = mat()) const;

    /**
     * @brief Applies a transformation to all vertices in the mesh.
     *
     * Each vertex position is updated by multiplying it with the given matrix.
     * Additionally, the cached normals for all faces are invalidated to ensure
     * correct recomputation on the next call.
     *
     * @param matrix The 4x4 transformation matrix.
     */
    void transform(const Matrix4x4& matrix);

    /**
     * @brief Computes and returns the center of the mesh.
     *
     * The center is computed as the average of all vertex positions.
     * The value is cached until the mesh is modified.
     *
     * @return The center point as a vec3.
     */
    vec3 getCenter() const;

    // Euler Operators

    /**
     * @brief Computes the Euler characteristic (V - E + F).
     * @return The Euler characteristic.
     */
    int getEulerCharacteristic() const;

    /**
    * @brief Checks if the Euler characteristic is valid (equal to 2).
    * @return True if the Euler characteristic is valid, false otherwise.
    */
    bool isEulerCharacteristicValid() const;

    bool areVerticesInSameFace(Vertex* v1, Vertex* v2, Vertex* v3) const;

    void _setWingPointers(Edge* edge, Face* face);

    Vertex* MVE(Vertex* existingVertex, const vec3& newVertexPos);

    void MEF(Vertex* v1, Vertex* v2, Vertex* v3);
private:
    mutable vec3 centerCache;
    mutable bool centerCacheValid = false;
};

/**
 * @brief Factory for creating primitive WingedEdge meshes.
 */
class PrimitiveFactory {
public:
    /**
     * @brief Creates a tetrahedron mesh.
     * @return Unique pointer to the WingedEdge mesh.
     */
    static std::unique_ptr<WingedEdge> createTetrahedron();

    /**
     * @brief Creates a box mesh given minimum and maximum vertices.
     * @param vmin The minimum vertex (corner).
     * @param vmax The maximum vertex (corner).
     * @return Unique pointer to the WingedEdge mesh.
     */
    static std::unique_ptr<WingedEdge> createBox(const vec3& vmin, const vec3& vmax);

    /**
     * @brief Creates a box mesh given minimum and maximum vertices.
     * @param vmin The minimum vertex (corner).
     * @param vmax The maximum vertex (corner).
     * @return Unique pointer to the WingedEdge mesh.
     */
    static std::unique_ptr<WingedEdge> createBoxEuler(const vec3& vmin, const vec3& vmax);

    /**
     * @brief Creates a sphere mesh.
     * @param center Center of the sphere.
     * @param radius Radius of the sphere.
     * @param latDivisions Number of latitude divisions.
     * @param longDivisions Number of longitude divisions.
     * @return Unique pointer to the WingedEdge mesh.
     */
    static std::unique_ptr<WingedEdge> createSphere(const vec3& center, float radius, int latDivisions = 12, int longDivisions = 24);

    static std::unique_ptr<WingedEdge> createCylinder(const vec3& center, float radius, float height, int radialDivisions = 24, int heightDivisions = 1);

private:
    /**
     * @brief Splits a four-vertex polygon (quad) into two CCW triangles.
     *
     *        v0 ---- v1
     *         |      |
     *         |      |
     *        v3 ---- v2
     *
     * By default, the "loop" is [v0, v1, v2, v3], which is assumed to be CCW
     * when viewed from the outside. The two triangles become:
     *   - (v0, v1, v2)
     *   - (v0, v2, v3)
     *
     * @param mesh The WingedEdge mesh where new faces are added.
     * @param v0   Quad vertex 0.
     * @param v1   Quad vertex 1.
     * @param v2   Quad vertex 2.
     * @param v3   Quad vertex 3.
     */
    static void makeQuadAsTriangles(WingedEdge& mesh, Vertex* v0, Vertex* v1, Vertex* v2, Vertex* v3, bool reverse = false);

    /**
     * @brief Creates a set of quads (two triangles each) between two rings of vertices.
     *
     * ring1[i] connects to ring2[i] and ring2[next], ring1[next], etc.
     * For a typical cylinder side, ring1 might be the lower ring, ring2 the upper ring,
     * both in CCW order (viewed from outside).
     *
     * @param mesh   The WingedEdge mesh where new faces are added.
     * @param ring1  First ring of vertices (e.g. bottom row).
     * @param ring2  Second ring of vertices (e.g. next row up).
     */
    static void makeQuadStrip(WingedEdge& mesh, const std::vector<Vertex*>& ring1, const std::vector<Vertex*>& ring2);

    /**
     * @brief Creates a fan of triangles from a center vertex and a loop of vertices.
     *
     * The orientation is determined by the order of the ring.
     * For a top cap (looking down from above), ensure the ring is CCW
     * so the normal is upward under the right-hand rule.
     *
     * @param mesh    The WingedEdge mesh where new faces are added.
     * @param center  The center vertex for the fan.
     * @param ring    The loop of vertices forming the outer boundary.
     */
    static void makeFan(WingedEdge& mesh, Vertex* center, const std::vector<Vertex*>& ring, bool reverse = false);
};

/**
 * @brief Collection of WingedEdge meshes for scene management.
 */
class MeshCollection {
public:
    /**
     * @brief Adds a mesh to the collection.
     * @param mesh Unique pointer to the WingedEdge mesh.
     * @param name Optional name for the mesh.
     * @param autoRename If true, automatically renames the mesh if a name collision occurs.
     */
    void addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name = "", bool autoRename = false);

    /**
     * @brief Removes a mesh from the collection by index.
     * @param index Index of the mesh to remove.
     */
    void removeMesh(size_t index);

    /**
     * @brief Gets a constant pointer to a mesh by index.
     * @param index Index of the mesh.
     * @return Constant pointer to the WingedEdge mesh.
     */
    const WingedEdge* getMesh(size_t index) const;

    /**
     * @brief Gets a pointer to a mesh by index.
     * @param index Index of the mesh.
     * @return Pointer to the WingedEdge mesh.
     */
    WingedEdge* getMesh(size_t index);

    /**
     * @brief Gets a constant pointer to a mesh by name.
     * @param name Name of the mesh.
     * @return Constant pointer to the WingedEdge mesh, or nullptr if not found.
     */
    const WingedEdge* getMeshByName(const std::string& name) const;

    /**
     * @brief Gets the name of a mesh.
     * @param mesh Pointer to the WingedEdge mesh.
     * @return Name of the mesh.
     */
    std::string getMeshName(const WingedEdge* mesh) const;

    /**
     * @brief Gets a pointer to a mesh by name.
     * @param name Name of the mesh.
     * @return Pointer to the WingedEdge mesh, or nullptr if not found.
     */
    WingedEdge* getMeshByName(const std::string& name);

    /**
     * @brief Returns the number of meshes in the collection.
     * @return Mesh count.
     */
    size_t getMeshCount() const;

    /**
     * @brief Adds a Winged Edge mesh to SceneManager for rendering.
     * @param world The scene manager.
     * @param meshName Name of the mesh.
     * @param material Material for rendering.
     * @return Object ID for the rendered mesh.
     */
    ObjectID addMeshToScene(SceneManager& world, const std::string& meshName, const mat& material);

    /**
     * @brief Removes a Winged Edge mesh from SceneManager.
     * @param world The scene manager.
     * @param meshName Name of the mesh.
     */
    void removeMeshFromScene(SceneManager& world, const std::string& meshName);

    /**
     * @brief Updates a mesh rendering in the scene.
     * @param world The scene manager.
     * @param meshName Name of the mesh.
     * @param material Material for rendering.
     * @return Object ID for the updated rendered mesh.
     */
    ObjectID updateMeshRendering(SceneManager& world, const std::string& meshName, const mat& material);

    /**
     * @brief Clears all meshes from the collection.
     */
    void clear();

    /**
     * @brief Prints information about the mesh collection.
     */
    void printInfo() const;

    /**
     * @brief Traverses all meshes in the collection.
     */
    void traverseMeshes() const;

    /**
     * @brief Transforms a WingedEdge mesh and updates its rendering in the scene.
     *
     * @param name The name of the mesh to transform.
     * @param matrix The transformation matrix to apply.
     * @param world The scene manager to update the mesh rendering.
     */
    void transformMesh(const std::string& name, const Matrix4x4& matrix, SceneManager& world, const mat& material);

    ObjectID getObjectID(const std::string& meshName) const;

private:
    std::vector<std::unique_ptr<WingedEdge>> meshes_;
    std::unordered_map<std::string, size_t> nameToIndexMap_;
    std::unordered_map<std::string, ObjectID> meshToWorldID_;
};

#endif // WINGED_EDGE_H
