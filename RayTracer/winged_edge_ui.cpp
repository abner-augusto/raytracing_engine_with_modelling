#include "winged_edge_ui.h"
#include "imgui.h"
#include <format>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <string>

//-------------------------------------------------------------------
// Helper methods implementation
//-------------------------------------------------------------------
std::string WingedEdgeImGui::getEdgeDisplayString(const Edge* e) {
    if (!e) return "nullptr";
    return std::format("e{}: ({:.2f},{:.2f},{:.2f}) -> ({:.2f},{:.2f},{:.2f})",
        e->index,
        e->origin->pos.x(), e->origin->pos.y(), e->origin->pos.z(),
        e->destination->pos.x(), e->destination->pos.y(), e->destination->pos.z());
}

std::string WingedEdgeImGui::getFaceDisplayString(const Face* f) {
    if (!f) return "nullptr";
    std::string result = std::format("f{}: ", f->index);
    // Use the boundaryEdges vector directly, no need for getBoundary()
    for (size_t i = 0; i < f->boundaryEdges.size(); i++) {
        result += std::format("e{}", f->boundaryEdges[i]->index);
        if (i < f->boundaryEdges.size() - 1)
            result += ", ";
    }
    vec3 normal = f->normal();  // Call the normal() method
    result += std::format(" | Normal: ({:.2f},{:.2f},{:.2f})",
        normal.x(), normal.y(), normal.z());
    return result;
}

//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
WingedEdgeImGui::WingedEdgeImGui(MeshCollection* collection)
    : meshCollection(collection), selectedMeshIndex(-1),  // Initialize to -1 (no selection)
    selectedEdgeIndex(-1), selectedFaceIndex(-1), selectedVertexIndex(-1),
    showEdgeDetails(false), showFaceDetails(false)
{
}

//-------------------------------------------------------------------
// Main render method
//-------------------------------------------------------------------
void WingedEdgeImGui::render() {
    if (!meshCollection) return;

    ImGui::Begin("WingedEdge Mesh Explorer");
    renderMeshList();
    ImGui::End();

    // Check for valid selectedMeshIndex *before* accessing the mesh.
    if (selectedMeshIndex >= 0 && selectedMeshIndex < static_cast<int>(meshCollection->getMeshCount())) {
        renderMeshDetails();
    }
}

//-------------------------------------------------------------------
// Mesh List Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderMeshList() {
    ImGui::Text("Mesh Collection (%zu meshes)", meshCollection->getMeshCount());
    ImGui::Separator();

    ImGui::BeginChild("MeshList", ImVec2(0, 150), true);
    if (meshCollection->getMeshCount() == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No meshes in collection");
    }
    else {
        for (size_t i = 0; i < meshCollection->getMeshCount(); i++) {
            const WingedEdge* mesh = meshCollection->getMesh(i);
            std::string meshName = meshCollection->getMeshName(mesh);
            if (meshName.empty()) {
                meshName = "Mesh " + std::to_string(i);
            }
            meshName += std::format(" - V:{} E:{} F:{}",
                mesh->vertices.size(), mesh->edges.size(), mesh->faces.size());
            if (ImGui::Selectable(meshName.c_str(), selectedMeshIndex == static_cast<int>(i))) {
                selectedMeshIndex = static_cast<int>(i);
                // Reset selections when a new mesh is selected.
                selectedEdgeIndex = -1;
                selectedFaceIndex = -1;
                selectedVertexIndex = -1;
                showEdgeDetails = false;
                showFaceDetails = false;
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Add Tetrahedron")) {
        auto newMesh = PrimitiveFactory::createTetrahedron();
        meshCollection->addMesh(std::move(newMesh),
            "Tetrahedron_" + std::to_string(meshCollection->getMeshCount()));
    }
    ImGui::SameLine();
    // Check mesh count *before* checking the index.
    if (meshCollection->getMeshCount() > 0 && ImGui::Button("Remove Selected") && selectedMeshIndex >= 0) {
        meshCollection->removeMesh(selectedMeshIndex);
        // Adjust selectedMeshIndex if we removed the last element.
        if (selectedMeshIndex >= static_cast<int>(meshCollection->getMeshCount())) {
            selectedMeshIndex = static_cast<int>(meshCollection->getMeshCount()) - 1;
        }
        // Reset sub-selections.
        selectedEdgeIndex = -1;
        selectedFaceIndex = -1;
        selectedVertexIndex = -1;
        showEdgeDetails = false;
        showFaceDetails = false;
    }
}

//-------------------------------------------------------------------
// Mesh Details Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderMeshDetails() {
    // Get a *non-const* pointer, as we might modify the mesh later.
    WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
    if (!mesh) return; // Important: Check if mesh is valid.

    std::string title = "Mesh Details";
    std::string meshName = meshCollection->getMeshName(mesh);
    if (!meshName.empty()) {
        title += " - " + meshName;
    }
    ImGui::Begin(title.c_str());
    if (ImGui::BeginTabBar("MeshTabs")) {
        if (ImGui::BeginTabItem("Vertices")) {
            renderVerticesTab(mesh);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Edges")) {
            renderEdgesTab(mesh);
            ImGui::Separator();
            // Check for valid selectedEdgeIndex *before* accessing.
            if (selectedEdgeIndex >= 0 && selectedEdgeIndex < static_cast<int>(mesh->edges.size())) {
                Edge* e = mesh->edges[selectedEdgeIndex].get();
                std::string headerText = std::format("Edge Details (e{})", e->index);
                ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(70, 70, 120, 255));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(90, 90, 150, 255));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(100, 100, 170, 255));
                if (ImGui::CollapsingHeader(headerText.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::PopStyleColor(3);
                    renderEdgeDetails(mesh);
                }
                else {
                    ImGui::PopStyleColor(3);
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Faces")) {
            renderFacesTab(mesh);
            ImGui::Separator();
            // Check for valid selectedFaceIndex *before* accessing.
            if (selectedFaceIndex >= 0 && selectedFaceIndex < static_cast<int>(mesh->faces.size())) {
                Face* f = mesh->faces[selectedFaceIndex].get();
                std::string headerText = std::format("Face Details (f{})", f->index);
                ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(70, 120, 70, 255));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(90, 150, 90, 255));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(100, 170, 100, 255));
                if (ImGui::CollapsingHeader(headerText.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::PopStyleColor(3);
                    renderFaceDetails(mesh);
                }
                else {
                    ImGui::PopStyleColor(3);
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

//-------------------------------------------------------------------
// Vertices Tab (with new incident pointer details)
//-------------------------------------------------------------------

void WingedEdgeImGui::renderVerticesTab(const WingedEdge* mesh) {
    ImGui::Text("Vertices (%zu)", mesh->vertices.size());
    ImGui::Separator();

    ImGui::BeginChild("VerticesList", ImVec2(0, 120), true);
    for (const auto& vertex : mesh->vertices) {
        // Use ->index, not .index
        std::string vertexStr = std::format("v{}: ({:.4f}, {:.4f}, {:.4f})",
            vertex->index, vertex->pos.x(), vertex->pos.y(), vertex->pos.z());

        if (ImGui::Selectable(vertexStr.c_str(), selectedVertexIndex == vertex->index)) {
            selectedVertexIndex = vertex->index;
        }
    }
    ImGui::EndChild();

    // Display details for the selected vertex.
    // Iterate and check index; don't assume selectedVertexIndex is in bounds.
    for (const auto& vertex : mesh->vertices)
    {
        if (vertex->index == selectedVertexIndex)
        {
            ImGui::Separator();
            ImGui::Text("Vertex Details:");
            std::string positionText = std::format("Position: ({:.4f}, {:.4f}, {:.4f})", vertex->pos.x(), vertex->pos.y(), vertex->pos.z());
            ImGui::Text("%s", positionText.c_str());


            if (vertex->incidentEdge) {
                std::string edgeStr = getEdgeDisplayString(vertex->incidentEdge);
                ImGui::Text("Incident Edge: %s", edgeStr.c_str());
            }
            else {
                ImGui::Text("Incident Edge: nullptr");
            }
        }
    }
}


//-------------------------------------------------------------------
// Edges Tab
//-------------------------------------------------------------------
void WingedEdgeImGui::renderEdgesTab(WingedEdge* mesh) { // Non-const pointer
    ImGui::Text("Edges (%zu)", mesh->edges.size());
    ImGui::Separator();

    ImGui::BeginChild("EdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->edges.size(); i++) {
        const auto& e = mesh->edges[i];
        std::string edgeStr = getEdgeDisplayString(e.get());  // Use get() for raw pointer
        if (ImGui::Selectable(edgeStr.c_str(), selectedEdgeIndex == static_cast<int>(i))) {
            selectedEdgeIndex = static_cast<int>(i);
            selectedEdgeLoopEdges.clear();  // Clear previous loop selection.
            selectedEdgeLoopEdges.push_back(mesh->edges[selectedEdgeIndex]); // Add selected edge
            showEdgeDetails = true;
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Full Edge Loop")) {
        selectLinkedEdgeLoop(mesh, false); // Now takes WingedEdge*
    }
    ImGui::SameLine();
    if (ImGui::Button("CW")) {
        selectLinkedEdgeLoop(mesh, true);  // Now takes WingedEdge*
    }
    ImGui::SameLine();
    if (ImGui::Button("CCW")) {
        selectLinkedEdgeLoopCounterclockwise(mesh); // Now takes WingedEdge*
    }
    ImGui::SameLine();
    if (ImGui::Button("By Vertex")) {
        selectEdgeChainByVertex(mesh); // New function for vertex-based edge selection.
    }
}

//-------------------------------------------------------------------
// Faces Tab
//-------------------------------------------------------------------
void WingedEdgeImGui::renderFacesTab(const WingedEdge* mesh) {
    ImGui::Text("Faces (%zu)", mesh->faces.size());
    ImGui::Separator();

    ImGui::BeginChild("FacesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->faces.size(); i++) {
        const auto& f = mesh->faces[i];
        std::string faceStr = getFaceDisplayString(f.get()); // Use get()
        if (ImGui::Selectable(faceStr.c_str(), selectedFaceIndex == static_cast<int>(i))) {
            selectedFaceIndex = static_cast<int>(i);
            showFaceDetails = true;
        }
    }
    ImGui::EndChild();
}

//-------------------------------------------------------------------
// Edge Details Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderEdgeDetails(const WingedEdge* mesh) {
    // Always check for valid index before accessing.
    if (selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size())) return;
    Edge* e = mesh->edges[selectedEdgeIndex].get();

    ImGui::Indent();
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "Origin:");
    ImGui::SameLine();
    ImGui::Text("(%.4f, %.4f, %.4f)",
        e->origin->pos.x(), e->origin->pos.y(), e->origin->pos.z());

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "Destination:");
    ImGui::SameLine();
    ImGui::Text("(%.4f, %.4f, %.4f)",
        e->destination->pos.x(), e->destination->pos.y(), e->destination->pos.z());

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Wing Pointers:");

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255));
    if (ImGui::TreeNode("CCW Left Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->counterClockwiseLeftEdge).c_str()); // Correct member name
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255));
    if (ImGui::TreeNode("CCW Right Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->counterClockwiseRightEdge).c_str()); // Correct member name
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255));
    if (ImGui::TreeNode("CW Left Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->clockwiseLeftEdge).c_str()); // Correct member name
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255));
    if (ImGui::TreeNode("CW Right Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->clockwiseRightEdge).c_str()); // Correct member name
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Adjacent Faces:");
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255));
    if (ImGui::TreeNode("Left Face")) {
        ImGui::PopStyleColor();
        // Use weak_ptr::lock() to safely access the shared_ptr.
        Face* lf = e->leftFace.expired() ? nullptr : e->leftFace.lock().get();
        ImGui::Text("%s", getFaceDisplayString(lf).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255));
    if (ImGui::TreeNode("Right Face")) {
        ImGui::PopStyleColor();
        Face* rf = e->rightFace.expired() ? nullptr : e->rightFace.lock().get();
        ImGui::Text("%s", getFaceDisplayString(rf).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }
    ImGui::Unindent();
}

//-------------------------------------------------------------------
// Face Details Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderFaceDetails(const WingedEdge* mesh) {
    // Always check index validity.
    if (selectedFaceIndex < 0 || selectedFaceIndex >= static_cast<int>(mesh->faces.size())) return;
    Face* f = mesh->faces[selectedFaceIndex].get();

    ImGui::Indent();
    vec3 normal = f->normal(); // Call the normal() method.
    ImGui::Text("Normal: (%.4f, %.4f, %.4f)", normal.x(), normal.y(), normal.z());
    ImGui::Separator();
    ImGui::Text("Boundary Edges:");
    // Use f->boundaryEdges directly.
    ImGui::BeginChild("FaceEdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < f->boundaryEdges.size(); i++) {
        Edge* e = f->boundaryEdges[i];
        std::string edgeStr = std::format("{}. {}", i + 1, getEdgeDisplayString(e));
        if (ImGui::Selectable(edgeStr.c_str())) {
            // Find the index of the selected edge within the mesh's edge list.
            for (size_t j = 0; j < mesh->edges.size(); j++) {
                if (mesh->edges[j].get() == e) {
                    selectedEdgeIndex = static_cast<int>(j);
                    break; // Important: Exit inner loop once found.
                }
            }
        }
    }
    ImGui::EndChild();
    ImGui::Unindent();
}

//-------------------------------------------------------------------
// Selection & Linked Edge Loop Methods
//-------------------------------------------------------------------
void WingedEdgeImGui::updateSelection(WingedEdge* selectedMesh, std::shared_ptr<Edge> selectedEdge) {
    if (!selectedMesh || !selectedEdge)
        return;

    // Find the mesh and edge indices.  Iterate; don't assume.
    for (size_t i = 0; i < meshCollection->getMeshCount(); i++) {
        if (meshCollection->getMesh(i) == selectedMesh) {
            selectedMeshIndex = i;
            for (size_t j = 0; j < selectedMesh->edges.size(); j++) {
                if (selectedMesh->edges[j] == selectedEdge) {
                    selectedEdgeIndex = static_cast<int>(j);
                    break;  // Exit inner loop when found.
                }
            }
            break; // Exit outer loop when mesh is found.
        }
    }
}

std::shared_ptr<Edge> WingedEdgeImGui::getSelectedEdge() const {
    // Check all conditions *before* accessing.
    if (selectedMeshIndex >= 0 && selectedMeshIndex < static_cast<int>(meshCollection->getMeshCount())) {
        WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
        if (selectedEdgeIndex >= 0 && selectedEdgeIndex < static_cast<int>(mesh->edges.size())) {
            return mesh->edges[selectedEdgeIndex];
        }
    }
    return nullptr; // Return nullptr if not found.
}
void WingedEdgeImGui::selectLinkedEdgeLoop(WingedEdge* mesh, bool clockwiseOnly) {
    if (!mesh || selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size()))
        return; // Check for valid mesh and index.

    selectedEdgeLoopEdges.clear();
    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    Edge* currentRaw = startEdge.get(); // Use raw pointer for traversal
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;

    // Traverse clockwise using the clockwiseRightEdge pointer.
    while (currentRaw->clockwiseRightEdge && currentRaw->clockwiseRightEdge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->clockwiseRightEdge->index) > 0)
            break;  // Prevent infinite loops.
        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<Edge>& e) {
                return e.get() == currentRaw->clockwiseRightEdge;
            });
        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->clockwiseRightEdge; // Move to the next edge.
            logStream << "->e" << (*it)->index;
        }
        else {
            break; // Stop if edge not found in the mesh's edge list.
        }
    }

    if (!clockwiseOnly) {
        // Traverse counter-clockwise (if not clockwiseOnly)
        visitedEdges.clear(); // Reset visited edges for the other direction.
        visitedEdges.insert(startEdge->index);
        currentRaw = startEdge.get();
        std::vector<std::shared_ptr<Edge>> reverseEdges; // Store edges for reversal.
        std::ostringstream reverseLogStream; // Separate log stream for counter-clockwise.

        while (currentRaw->counterClockwiseLeftEdge && currentRaw->counterClockwiseLeftEdge != startEdge.get())
        {
            if (visitedEdges.count(currentRaw->counterClockwiseLeftEdge->index) > 0)
                break;  // Prevent infinite loops.

            auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
                [currentRaw](const std::shared_ptr<Edge>& e)
                {
                    return e.get() == currentRaw->counterClockwiseLeftEdge;
                });
            if (it != mesh->edges.end())
            {
                reverseEdges.push_back(*it);
                visitedEdges.insert((*it)->index);
                currentRaw = currentRaw->counterClockwiseLeftEdge;  //Move to next edge.
                reverseLogStream << "e" << (*it)->index << "->";
            }
            else
            {
                break; // Stop if edge not found in the mesh's edge list
            }
        }

        std::reverse(reverseEdges.begin(), reverseEdges.end()); // Reverse the counter-clockwise edges.
        selectedEdgeLoopEdges.insert(selectedEdgeLoopEdges.begin(), reverseEdges.begin(), reverseEdges.end());// Add to the beginning.
        std::cout << "Edge Loop: " << reverseLogStream.str() << logStream.str() << std::endl;
    }
    else
    {
        std::cout << "Edge Loop (Clockwise Only): " << logStream.str() << std::endl;

    }
}

void WingedEdgeImGui::selectLinkedEdgeLoopCounterclockwise(WingedEdge* mesh) {
    if (!mesh || selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size())) //validate inputs
        return;

    selectedEdgeLoopEdges.clear();
    auto startEdge = mesh->edges[selectedEdgeIndex]; //get our starting edge
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;  //keep track of visited edges to prevent loops
    visitedEdges.insert(startEdge->index);

    Edge* currentRaw = startEdge.get(); //use a raw pointer to simplify traversal
    std::ostringstream logStream; //log the traversal to the console
    logStream << "e" << startEdge->index;

    //traverse counter clockwise
    while (currentRaw->counterClockwiseLeftEdge && currentRaw->counterClockwiseLeftEdge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->counterClockwiseLeftEdge->index) > 0)
            break;  // Prevent infinite loops.

        // Find shared pointer in the vector
        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<Edge>& e) {
                return e.get() == currentRaw->counterClockwiseLeftEdge;
            });
        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->counterClockwiseLeftEdge; //advance to next edge
            logStream << "->e" << (*it)->index; //add edge index to log
        }
        else {
            break; // Stop if edge not found in the mesh's edge list
        }
    }
    std::cout << "Edge Loop (Counterclockwise Only): " << logStream.str() << std::endl;
}

void WingedEdgeImGui::selectEdgeChainByVertex(WingedEdge* mesh) {
    if (!mesh || selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size()))
        return; // Validate mesh and index.

    selectedEdgeLoopEdges.clear();
    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    Edge* currentRaw = startEdge.get(); // Begin traversal from the selected edge.
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;

    bool closedLoop = false;

    // Continue searching for an edge whose origin equals the destination of the current edge.
    while (true) {
        bool foundNext = false;
        // Iterate through all edges in the mesh.
        for (const auto& edge : mesh->edges) {
            // Check if the current edge's destination matches this edge's origin.
            if (edge->origin == currentRaw->destination) {
                // If this edge is the starting edge, we've closed the loop.
                if (edge->index == startEdge->index) {
                    logStream << "->e" << edge->index << " (loop closed)";
                    closedLoop = true;
                    foundNext = false;
                    break;
                }
                // Only add edges that haven't been visited yet.
                if (visitedEdges.find(edge->index) == visitedEdges.end()) {
                    selectedEdgeLoopEdges.push_back(edge);
                    visitedEdges.insert(edge->index);
                    currentRaw = edge.get();
                    logStream << "->e" << edge->index;
                    foundNext = true;
                    break;
                }
            }
        }
        if (!foundNext)
            break;
    }

    if (closedLoop)
        std::cout << "Edge Chain (Closed Loop): " << logStream.str() << std::endl;
    else
        std::cout << "Edge Chain: " << logStream.str() << std::endl;
}
