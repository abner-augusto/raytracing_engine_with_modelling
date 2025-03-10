#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <unordered_map>
#include "vec3.h"
#include "mesh.h"
#include "winged_edge.h"

/*-------------------------------------------------------------------
  Face Methods
-------------------------------------------------------------------*/

vec3 Face::normal() const {
    if (!normalCached) {
        if (vertices.size() < 3)
            cachedNormal = vec3(0, 0, 0);
        else {
            vec3 v0 = vertices[0]->pos;
            vec3 v1 = vertices[1]->pos;
            vec3 v2 = vertices[2]->pos;
            vec3 cross_product = cross(v1 - v0, v2 - v0);

            // Check for near-zero cross product.
            if (cross_product.length_squared() < 1e-12) { // Or another suitable small value
                cachedNormal = vec3(0, 0, 0); // Or some default normal, or throw an exception.
            }
            else {
                cachedNormal = unit_vector(cross_product);
            }
        }
        normalCached = true;
    }
    return cachedNormal;
}

/*-------------------------------------------------------------------
  WingedEdge Methods
-------------------------------------------------------------------*/

std::shared_ptr<Edge> WingedEdge::findOrCreateEdge(Vertex* vertex1, Vertex* vertex2) {
    EdgeKey key(vertex1->index, vertex2->index);
    auto it = edgeLookup.find(key);
    if (it != edgeLookup.end()) {
        return it->second;
    }
    auto newEdge = std::make_shared<Edge>(vertex1, vertex2);
    newEdge->index = static_cast<int>(edges.size());
    edges.push_back(newEdge);
    edgeLookup.emplace(key, newEdge);

    // Set the incident edge pointers if not already assigned.
    if (vertex1->incidentEdge == nullptr)
        vertex1->incidentEdge = newEdge.get();
    if (vertex2->incidentEdge == nullptr)
        vertex2->incidentEdge = newEdge.get();

    return newEdge;
}

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
        Vertex* vertex1 = boundary[i];
        Vertex* vertex2 = boundary[(i + 1) % n];
        auto edge = findOrCreateEdge(vertex1, vertex2);

        // Assign the face to the edge.
                // Use raw pointer comparison.
        if (edge->leftFace.expired())
            edge->leftFace = newFace;
        else if (edge->rightFace.expired())
            edge->rightFace = newFace;
        else
        {
            // Check if faces are the same
            if (edge->leftFace.lock().get() != newFace.get() && edge->rightFace.lock().get() != newFace.get())
            {
                throw std::runtime_error("Edge already has two adjacent faces.");
            }
        }
        newFace->boundaryEdges.push_back(edge.get());
    }

    // Set the starting edge for the face.
    if (!newFace->boundaryEdges.empty())
        newFace->edge = newFace->boundaryEdges[0];

    return newFace;
}

void WingedEdge::setupWingedEdgePointers() {
    // Reset all wing pointers.
    for (auto& e : edges) {
        e->counterClockwiseLeftEdge = nullptr;
        e->clockwiseLeftEdge = nullptr;
        e->counterClockwiseRightEdge = nullptr;
        e->clockwiseRightEdge = nullptr;
    }

    // For each face, traverse its boundary (accessed directly via boundaryEdges) and set pointers.
    for (auto& f : faces) {
        auto& boundary = f->boundaryEdges;
        size_t n = boundary.size();
        for (size_t i = 0; i < n; i++) {
            Edge* current = boundary[i];
            Edge* next = boundary[(i + 1) % n];

            // Set the clockwise pointer based on face assignment.
            if (current->leftFace.lock().get() == f.get())
                current->clockwiseLeftEdge = next;
            else if (current->rightFace.lock().get() == f.get())
                current->clockwiseRightEdge = next;

            // Set the counterclockwise pointer.
            Edge* prev = boundary[(i + n - 1) % n];
            if (current->leftFace.lock().get() == f.get())
                current->counterClockwiseLeftEdge = prev;
            else if (current->rightFace.lock().get() == f.get())
                current->counterClockwiseRightEdge = prev;
        }
    }
}

void WingedEdge::printInfo() const {
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
        std::cout << "Boundary edges = ";
        for (Edge* e : f->boundaryEdges) {
            std::cout << "e" << e->index << " ";
        }
        vec3 n = f->normal();
        std::cout << " | Normal = (" << n.x() << ", " << n.y() << ", " << n.z() << ")" << std::endl;
    }

     // Print Euler characteristic.
    std::cout << "Euler Characteristic: " << getEulerCharacteristic() << std::endl;
    std::cout << "Euler Characteristic Valid: " << (isEulerCharacteristicValid() ? "Yes" : "No") << std::endl;
}

void WingedEdge::traverseMesh() const {
    std::cout << "\nMesh Traversal:" << std::endl;
    for (const auto& f : faces) {
        std::cout << "Face " << f->index << " vertices: ";
        const auto& verts = f->vertices;
        for (size_t i = 0; i < verts.size(); i++) {
            std::cout << "(" << verts[i]->pos.x() << ", " << verts[i]->pos.y() << ", " << verts[i]->pos.z() << ")";
            if (i < verts.size() - 1)
                std::cout << " -> ";
        }
        std::cout << std::endl;
    }
}

std::shared_ptr<Mesh> WingedEdge::toMesh(const mat& material) const {
    auto mesh = std::make_shared<Mesh>();

    // Map from WingedEdge vertex pointer to a unique index in the Mesh vertex list.
    std::unordered_map<const Vertex*, size_t> vertexMap;
    std::vector<vec3> positions;

    // First pass: Register every unique vertex from every face.
    for (const auto& face : faces) {
        for (const auto& v : face->vertices) {
            if (vertexMap.find(v) == vertexMap.end()) {
                size_t index = positions.size();
                vertexMap[v] = index;
                positions.push_back(v->pos);
            }
        }
    }

    // Second pass: Process each face and create triangles.
    for (const auto& face : faces) {
        const auto& verts = face->vertices;
        if (verts.size() >= 3) { // Handles faces with 3 or more vertices.
            // Triangulate the face (fan triangulation).
            for (size_t i = 1; i < verts.size() - 1; ++i) {
                size_t i0 = vertexMap.at(verts[0]);  // First vertex of the face.
                size_t i1 = vertexMap.at(verts[i]);    // Current vertex.
                size_t i2 = vertexMap.at(verts[i + 1]);// Next vertex.

                vec3 v0 = positions[i0];
                vec3 v1 = positions[i1];
                vec3 v2 = positions[i2];

                // Create a triangle using the cached normal from the Face.
                auto tri = std::make_shared<triangle>(v0, v1, v2, face->normal(), material);
                mesh->add_triangle(tri);
            }
        }
        else {
            std::cerr << "Warning: Face " << face->index << " has fewer than 3 vertices. Skipping." << std::endl;
        }
    }

    mesh->buildBVH();
    return mesh;
}

void WingedEdge::transform(const Matrix4x4& matrix) {
    // Update each vertex position.
    for (auto& vertexPtr : vertices) {
        vertexPtr->pos = matrix.transform_point(vertexPtr->pos);
    }
    // Invalidate Cached Value of Center
    centerCacheValid = false;

    // Invalidate cached normals in all faces.
    for (auto& facePtr : faces) {
        facePtr->invalidateCache();
    }
}

vec3 WingedEdge::getCenter() const {
    if (!centerCacheValid) {
        vec3 center(0, 0, 0);
        if (!vertices.empty()) {
            for (const auto& vertex : vertices) {
                center += vertex->pos;
            }
            centerCache = center / static_cast<float>(vertices.size());
        }
        else {
            centerCache = vec3(0, 0, 0);
        }
        centerCacheValid = true;
    }
    return centerCache;
}

// Euler Operators Implementations:

int WingedEdge::getEulerCharacteristic() const {
    return static_cast<int>(vertices.size()) - static_cast<int>(edges.size()) + static_cast<int>(faces.size());
}

bool WingedEdge::isEulerCharacteristicValid() const {
    return (getEulerCharacteristic() == 2);
}

bool WingedEdge::areVerticesInSameFace(Vertex* v1, Vertex* v2, Vertex* v3) const {
    for (const auto& face : faces) {
        const auto& verts = face->vertices;
        if (verts.size() == 3) { // Assuming only triangular faces
            bool foundV1 = false, foundV2 = false, foundV3 = false;
            for (const auto& v : verts) {
                if (v == v1) foundV1 = true;
                if (v == v2) foundV2 = true;
                if (v == v3) foundV3 = true;
            }
            if (foundV1 && foundV2 && foundV3) {
                return true; // All three vertices found in the same face
            }
        }
    }
    return false; // No such face found
}


// Helper function to set wing pointers incrementally *within* Euler operators.
void WingedEdge::_setWingPointers(Edge* edge, Face* face) {
    if (!edge || !face) return;

    // Find the position of 'edge' within the face's boundaryEdges.
    auto it = std::find(face->boundaryEdges.begin(), face->boundaryEdges.end(), edge);
    if (it == face->boundaryEdges.end()) {
        return; // Edge not part of this face.
    }

    size_t i = std::distance(face->boundaryEdges.begin(), it);
    size_t n = face->boundaryEdges.size();
    Edge* next = face->boundaryEdges[(i + 1) % n];
    Edge* prev = face->boundaryEdges[(i + n - 1) % n];

    // Set pointers based on face orientation (left or right).
    if (edge->leftFace.lock().get() == face) {
        edge->clockwiseLeftEdge = next;
        edge->counterClockwiseLeftEdge = prev;
    }
    else if (edge->rightFace.lock().get() == face) {
        edge->clockwiseRightEdge = next;
        edge->counterClockwiseRightEdge = prev;
    }
}


Vertex* WingedEdge::MEV(Vertex* existingVertex, const vec3& newVertexPos) {
    auto newVertex = std::make_unique<Vertex>(newVertexPos, static_cast<int>(vertices.size()));
    Vertex* rawNewVertex = newVertex.get();
    vertices.push_back(std::move(newVertex));

    if (existingVertex) { // If existingVertex is nullptr, we're creating the first vertex.
        findOrCreateEdge(existingVertex, rawNewVertex); // Edge creation is handled here.
    }
    return rawNewVertex; // Return the *raw* pointer.
}

void WingedEdge::MEF(Vertex* v1, Vertex* v2, Vertex* v3) {
    if (v1 == v2 || v1 == v3 || v2 == v3)
        throw std::runtime_error("MEF: Vertices must be distinct.");

    // Check for pre-existing faces.
    if (areVerticesInSameFace(v1, v2, v3))
        throw std::runtime_error("MEF: Vertices already form a face.");

    // Create the face and its edges.  Crucially, *all* connectivity is done here.
    auto newFace = createFace({ v1, v2, v3 });

    // Set the wing pointers *incrementally* for each edge of the new face.
    for (Edge* edge : newFace->boundaryEdges) {
        _setWingPointers(edge, newFace.get());
    }
}

/*-------------------------------------------------------------------
  PrimitiveFactory Implementation
-------------------------------------------------------------------*/

//Helpers
void PrimitiveFactory::makeFan(WingedEdge& mesh, Vertex* center, const std::vector<Vertex*>& ring, bool reverse)
{
    int n = static_cast<int>(ring.size());
    if (n < 3) return;

    for (int i = 0; i < n; i++)
    {
        int next = (i + 1) % n;
        if (reverse) {
            // Reverse the triangle order to flip the normal.
            mesh.createFace({ center, ring[i], ring[next] });
        }
        else {
            mesh.createFace({ center, ring[next], ring[i] });
        }
    }
}

void PrimitiveFactory::makeQuadStrip(WingedEdge& mesh,
    const std::vector<Vertex*>& ring1,
    const std::vector<Vertex*>& ring2)
{
    if (ring1.size() != ring2.size()) {
        throw std::runtime_error("makeQuadStrip: rings must have the same size");
    }
    int n = static_cast<int>(ring1.size());
    for (int i = 0; i < n; i++)
    {
        int next = (i + 1) % n;
        // Build two triangles per quad:
        // Triangle 1: bottom (ring1[i]), top (ring2[i]), top next (ring2[next])
        mesh.createFace({ ring1[i], ring2[i], ring2[next] });
        // Triangle 2: bottom (ring1[i]), top next (ring2[next]), bottom next (ring1[next])
        mesh.createFace({ ring1[i], ring2[next], ring1[next] });
    }
}

void PrimitiveFactory::makeQuadAsTriangles(WingedEdge& mesh,
    Vertex* v0,
    Vertex* v1,
    Vertex* v2,
    Vertex* v3,
    bool reverse)
{
    if (reverse) {
        // Reverse ordering flips the face normal.
        mesh.createFace({ v0, v3, v2 });
        mesh.createFace({ v0, v2, v1 });
    }
    else {
        mesh.createFace({ v0, v1, v2 });
        mesh.createFace({ v0, v2, v3 });
    }
}

// Primitives
std::unique_ptr<WingedEdge> PrimitiveFactory::createTetrahedron() {
    auto mesh = std::make_unique<WingedEdge>();

    // 1. Create the first vertex (v0).
    Vertex* v0 = mesh->MEV(nullptr, vec3(0.0, 0.0, 0.0));

    // 2. Add vertices and edges to create the first triangle (base).
    Vertex* v1 = mesh->MEV(v0, vec3(1.0, 0.0, 0.0));
    Vertex* v2 = mesh->MEV(v1, vec3(0.5, 0.0, 0.866025));
    mesh->MEF(v0, v1, v2);

    // 3. Add the apex vertex (v3) and connect it to the base.
    Vertex* v3 = mesh->MEV(v0, vec3(0.5, 0.816496, 0.288675));
    mesh->MEF(v0, v3, v1); // Connect v3 to v0 and v1.
    mesh->MEF(v1, v3, v2); // Connect v3 to v1 and v2.
    mesh->MEF(v2, v3, v0); // Connect v3 to v2 and v0.

    return mesh;
}

std::unique_ptr<WingedEdge> PrimitiveFactory::createBox(const vec3& vmin, const vec3& vmax) {
    auto mesh = std::make_unique<WingedEdge>();

    // Create the 8 vertices of the box.
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmin.x(), vmin.y(), vmin.z()), 0)); // v0
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmax.x(), vmin.y(), vmin.z()), 1)); // v1
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmax.x(), vmax.y(), vmin.z()), 2)); // v2
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmin.x(), vmax.y(), vmin.z()), 3)); // v3
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmin.x(), vmin.y(), vmax.z()), 4)); // v4
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmax.x(), vmin.y(), vmax.z()), 5)); // v5
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmax.x(), vmax.y(), vmax.z()), 6)); // v6
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(vmin.x(), vmax.y(), vmax.z()), 7)); // v7

    // Front face: v0, v1, v2, v3
    makeQuadAsTriangles(*mesh,
        mesh->vertices[0].get(),
        mesh->vertices[1].get(),
        mesh->vertices[2].get(),
        mesh->vertices[3].get(),
        false);

    // Back face: v4, v5, v6, v7
    makeQuadAsTriangles(*mesh,
        mesh->vertices[4].get(),
        mesh->vertices[5].get(),
        mesh->vertices[6].get(),
        mesh->vertices[7].get(),
        true);

    // Bottom face: v0, v1, v5, v4
    makeQuadAsTriangles(*mesh,
        mesh->vertices[0].get(),
        mesh->vertices[1].get(),
        mesh->vertices[5].get(),
        mesh->vertices[4].get(),
        true);

    // Top face: v3, v2, v6, v7
    makeQuadAsTriangles(*mesh,
        mesh->vertices[3].get(),
        mesh->vertices[2].get(),
        mesh->vertices[6].get(),
        mesh->vertices[7].get(),
        false);

    // Left face: v0, v3, v7, v4
    makeQuadAsTriangles(*mesh,
        mesh->vertices[0].get(),
        mesh->vertices[3].get(),
        mesh->vertices[7].get(),
        mesh->vertices[4].get());

    // Right face: v1, v2, v6, v5
    makeQuadAsTriangles(*mesh,
        mesh->vertices[1].get(),
        mesh->vertices[2].get(),
        mesh->vertices[6].get(),
        mesh->vertices[5].get(),
        true);

    mesh->setupWingedEdgePointers();
    return mesh;
}

std::unique_ptr<WingedEdge> PrimitiveFactory::createBoxEuler(const vec3& vmin, const vec3& vmax)
{
    auto mesh = std::make_unique<WingedEdge>();

    // 1. Create the first vertex (v0).
    Vertex* v0 = mesh->MEV(nullptr, vec3(vmin.x(), vmin.y(), vmin.z()));

    // 2. Create the other vertices of the front face, ensuring correct connectivity.
    Vertex* v1 = mesh->MEV(v0, vec3(vmax.x(), vmin.y(), vmin.z()));
    Vertex* v2 = mesh->MEV(v1, vec3(vmax.x(), vmax.y(), vmin.z()));
    Vertex* v3 = mesh->MEV(v2, vec3(vmin.x(), vmax.y(), vmin.z()));

    // 3. Front face: v0, v1, v2, v3 (Original: same winding)
    mesh->MEF(v0, v1, v2);
    mesh->MEF(v0, v2, v3);

    // 4. Create the back vertices.
    Vertex* v4 = mesh->MEV(v0, vec3(vmin.x(), vmin.y(), vmax.z()));
    Vertex* v5 = mesh->MEV(v1, vec3(vmax.x(), vmin.y(), vmax.z()));
    Vertex* v6 = mesh->MEV(v2, vec3(vmax.x(), vmax.y(), vmax.z()));
    Vertex* v7 = mesh->MEV(v3, vec3(vmin.x(), vmax.y(), vmax.z()));

    // 5. Back face: v4, v5, v6, v7 (Original: reversed winding)
    mesh->MEF(v4, v6, v5);
    mesh->MEF(v4, v7, v6);

    // 6. Bottom face: v0, v1, v5, v4 (Original: reversed winding)
    mesh->MEF(v0, v5, v1);
    mesh->MEF(v0, v4, v5);

    // 7. Top face: v3, v2, v6, v7 (Original: same winding)
    mesh->MEF(v3, v2, v6);
    mesh->MEF(v3, v6, v7);

    // 8. Left face: v0, v3, v7, v4 (Original: same winding)
    mesh->MEF(v0, v3, v7);
    mesh->MEF(v0, v7, v4);

    // 9. Right face: v1, v2, v6, v5 (Original: reversed winding)
    mesh->MEF(v1, v6, v2);
    mesh->MEF(v1, v5, v6);

    return mesh;
}

std::unique_ptr<WingedEdge> PrimitiveFactory::createSphere(const vec3& center, float radius, int latDivisions, int longDivisions) {
    auto mesh = std::make_unique<WingedEdge>();
    int index = 0;

    // Create the north pole vertex.
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(center.x(), center.y() + radius, center.z()), index++)); // North pole

    // Create vertices for intermediate latitude rings.
    std::vector<std::vector<int>> vertexIndices;
    for (int i = 1; i < latDivisions; i++) {
        float phi = pi * i / latDivisions;  // polar angle from north pole to south pole.
        std::vector<int> ring;
        for (int j = 0; j < longDivisions; j++) {
            float theta = 2.0f * pi * j / longDivisions;  // azimuthal angle.
            double x = center.x() + radius * sin(phi) * cos(theta);
            double y = center.y() + radius * cos(phi);
            double z = center.z() + radius * sin(phi) * sin(theta);
            mesh->vertices.push_back(std::make_unique<Vertex>(vec3(x, y, z), index));
            ring.push_back(index);
            index++;
        }
        vertexIndices.push_back(ring);
    }

    // Create the south pole vertex.
    int southPoleIndex = index;
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(center.x(), center.y() - radius, center.z()), index++));

    // Top cap: connect north pole to first ring.
    for (int j = 0; j < longDivisions; j++) {
        int next = (j + 1) % longDivisions;
        mesh->createFace({
            mesh->vertices[0].get(),  // north pole
            mesh->vertices[vertexIndices[0][next]].get(),
            mesh->vertices[vertexIndices[0][j]].get()
            });
    }

    // Middle bands: build quads split into two triangles.
    for (size_t i = 0; i < vertexIndices.size() - 1; i++) {
        for (int j = 0; j < longDivisions; j++) {
            int next = (j + 1) % longDivisions;
            int current1 = vertexIndices[i][j];
            int current2 = vertexIndices[i][next];
            int next1 = vertexIndices[i + 1][j];
            int next2 = vertexIndices[i + 1][next];

            // First triangle: (current1, current2, next1)
            mesh->createFace({
                mesh->vertices[current1].get(),
                mesh->vertices[current2].get(),
                mesh->vertices[next1].get()
                });
            // Second triangle: (current2, next2, next1)
            mesh->createFace({
                mesh->vertices[current2].get(),
                mesh->vertices[next2].get(),
                mesh->vertices[next1].get()
                });
        }
    }

    // Bottom cap: connect last ring to south pole.
    size_t lastRing = vertexIndices.size() - 1;
    for (int j = 0; j < longDivisions; j++) {
        int next = (j + 1) % longDivisions;
        mesh->createFace({
            mesh->vertices[vertexIndices[lastRing][j]].get(),
            mesh->vertices[vertexIndices[lastRing][next]].get(),
            mesh->vertices[southPoleIndex].get()
            });
    }

    mesh->setupWingedEdgePointers();
    return mesh;
}

std::unique_ptr<WingedEdge> PrimitiveFactory::createCylinder(const vec3& center, float radius, float height, int radialDivisions, int heightDivisions) {
    auto mesh = std::make_unique<WingedEdge>();
    int index = 0;
    double halfHeight = height / 2.0;
    double bottomY = center.y() - halfHeight;
    double topY = center.y() + halfHeight;

    // Compute the number of horizontal rings.
    int numRings = heightDivisions + 1;
    std::vector<std::vector<Vertex*>> rings(numRings);

    // Create rings of vertices along the height.
    for (int r = 0; r < numRings; r++) {
        double t = static_cast<double>(r) / heightDivisions;
        double y = bottomY + t * height;
        rings[r].resize(radialDivisions);
        for (int i = 0; i < radialDivisions; i++) {
            double angle = 2.0 * pi * i / radialDivisions;
            double x = center.x() + radius * std::cos(angle);
            double z = center.z() + radius * std::sin(angle);
            mesh->vertices.push_back(std::make_unique<Vertex>(vec3(x, y, z), index));
            rings[r][i] = mesh->vertices.back().get();
            index++;
        }
    }

    // Build the lateral surface using quad strips.
    for (int r = 0; r < heightDivisions; r++) {
        makeQuadStrip(*mesh, rings[r], rings[static_cast<std::vector<std::vector<Vertex*, std::allocator<Vertex*>>, std::allocator<std::vector<Vertex*, std::allocator<Vertex*>>>>::size_type>(r) + 1]);
    }

    // Build the bottom cap.
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(center.x(), bottomY + 0.001, center.z()), index));
    Vertex* bottomCenter = mesh->vertices.back().get();
    index++;
    makeFan(*mesh, bottomCenter, rings[0], true); // **Reverse for bottom cap**

    // Build the top cap.
    mesh->vertices.push_back(std::make_unique<Vertex>(vec3(center.x(), topY + 0.001, center.z()), index));
    Vertex* topCenter = mesh->vertices.back().get();
    index++;
    makeFan(*mesh, topCenter, rings[numRings - 1], false); // **Natural order for top cap**

    mesh->setupWingedEdgePointers();
    return mesh;
}

/*-------------------------------------------------------------------
  MeshCollection Implementation
-------------------------------------------------------------------*/

void MeshCollection::addMesh(std::unique_ptr<WingedEdge> mesh, const std::string& name, bool autoRename) {
    if (!name.empty()) {
        std::string finalName = name;
        if (nameToIndexMap_.count(finalName) > 0) {
            if (autoRename) {
                int count = 1;
                while (nameToIndexMap_.count(finalName + "_" + std::to_string(count)) > 0)
                    count++;
                finalName = finalName + "_" + std::to_string(count);
            }
            else {
                throw std::invalid_argument("Mesh with name '" + finalName + "' already exists.");
            }
        }
        nameToIndexMap_[finalName] = meshes_.size();
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

// Helper to add a mesh rendering to the world.
ObjectID MeshCollection::addMeshToScene(SceneManager& world, const std::string& meshName, const mat& material) {
    const WingedEdge* mesh = getMeshByName(meshName);
    if (!mesh)
        throw std::runtime_error("Mesh with name '" + meshName + "' not found.");

    // Convert the WingedEdge to a renderable triangle mesh.
    std::shared_ptr<Mesh> renderMesh = mesh->toMesh(material);
    ObjectID id = world.add(renderMesh);
    // Store the mapping from mesh name to world ObjectID.
    meshToWorldID_[meshName] = id;
    return id;
}

// Helper to remove a mesh rendering from the world.
void MeshCollection::removeMeshFromScene(SceneManager& world, const std::string& meshName) {
    auto it = meshToWorldID_.find(meshName);
    if (it != meshToWorldID_.end()) {
        world.remove(it->second);
        meshToWorldID_.erase(it);
    }
    else {
        std::cerr << "Mesh '" << meshName << "' is not present in the world." << std::endl;
    }
}

// Helper to update a mesh rendering in the world.
ObjectID MeshCollection::updateMeshRendering(SceneManager& world, const std::string& meshName, const mat& material) {
    removeMeshFromScene(world, meshName);  // Remove any existing rendering.
    return addMeshToScene(world, meshName, material);  // Add the updated rendering.
}

void MeshCollection::transformMesh(const std::string& name, const Matrix4x4& matrix, SceneManager& world, const mat& material) {
    // Retrieve the mesh by name
    WingedEdge* mesh = getMeshByName(name);

    if (!mesh) {
        std::cerr << "Error: Mesh '" << name << "' not found in collection." << std::endl;
        return;
    }

    // Apply the transformation to the mesh
    mesh->transform(matrix);

    // Update the rendering in the scene
    updateMeshRendering(world, name, material);
}

ObjectID MeshCollection::getObjectID(const std::string& meshName) const {
    auto it = meshToWorldID_.find(meshName);
    if (it != meshToWorldID_.end())
        return it->second;
    throw std::runtime_error("Mesh '" + meshName + "' is not present in the world.");
}
