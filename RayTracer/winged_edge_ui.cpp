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
WingedEdgeImGui::WingedEdgeImGui(MeshCollection* collection, SceneManager* world)
    : meshCollection(collection), sceneManager(world),  selectedMeshIndex(-1),  // Initialize to -1 (no selection)
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
    // Animation logic
    if (animationState == AnimationState::Playing) {
        animateCreateBox();
    }
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
    ImGui::SameLine();
    // Animate Create Box Button
    if (ImGui::Button("Animate Create Box")) {
        // Reset the mesh collection and start the animation.
        meshCollection->clear(); // Clear any existing meshes
        animationState = AnimationState::Playing;
        animationStep = 0; // Start from the first step
        animationStartTime = std::chrono::steady_clock::now(); // Capture start time

        // Create an initial empty mesh.
        auto newMesh = std::make_unique<WingedEdge>();
        meshCollection->addMesh(std::move(newMesh), "AnimatedBox");
        selectedMeshIndex = 0;  // Select the newly created mesh.

    }

    if (ImGui::Button("Print Mesh Info")) {
        if (selectedMeshIndex >= 0 && selectedMeshIndex < meshCollection->getMeshCount()) {
            const WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
            if (mesh) {
                mesh->printInfo();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Traverse Mesh")) {
        if (selectedMeshIndex >= 0 && selectedMeshIndex < meshCollection->getMeshCount()) {
            const WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
            if (mesh) {
                mesh->traverseMesh();
            }
        }
    }
}

//-------------------------------------------------------------------
// Mesh Details Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderMeshDetails() {
    // Get a non-const pointer, as we might modify the mesh later.
    WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
    if (!mesh) return; // Ensure the mesh is valid.

    std::string title = "Mesh Details";
    std::string meshName = meshCollection->getMeshName(mesh);
    if (!meshName.empty()) {
        title += " - " + meshName;
    }
    ImGui::Begin(title.c_str());

    if (ImGui::BeginTabBar("MeshTabs")) {
        // Vertices Tab
        if (ImGui::BeginTabItem("Vertices")) {
            ImGui::PushID("VerticesTab");
            renderVerticesTab(mesh);
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        // Edges Tab
        if (ImGui::BeginTabItem("Edges")) {
            ImGui::PushID("EdgesTab");
            renderEdgesTab(mesh);
            ImGui::Separator();
            // Check for valid selectedEdgeIndex before accessing.
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
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        // Faces Tab
        if (ImGui::BeginTabItem("Faces")) {
            ImGui::PushID("FacesTab");
            renderFacesTab(mesh);
            ImGui::Separator();
            // Check for valid selectedFaceIndex before accessing.
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
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        // Geometry Tab
        if (ImGui::BeginTabItem("Geometry")) {
            ImGui::PushID("GeometryTab");
            ShowWingedEdgeGeometryTab(mesh);
            ImGui::PopID();
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
            selectedEdgeLoopEdges.clear(); // Clear any previous selection
            for (Edge* edge : f->boundaryEdges) {
                auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
                    [edge](const std::shared_ptr<Edge>& e) { return e.get() == edge; });

                if (it != mesh->edges.end()) {
                    selectedEdgeLoopEdges.push_back(*it);
                }
            }
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
        if (ImGui::Selectable(edgeStr.c_str(), selectedEdgeIndex == e->index)) {
            // Find the index of the selected edge within the mesh's edge list.
            for (size_t j = 0; j < mesh->edges.size(); j++) {
                if (mesh->edges[j].get() == e) {
                    selectedEdgeIndex = static_cast<int>(j);
                    selectedEdgeLoopEdges.clear(); // Clear any previous selection
                    selectedEdgeLoopEdges.push_back(mesh->edges[j]); // Add the selected edge

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

// Geometry Tab for Winged Edge Meshes
void WingedEdgeImGui::ShowWingedEdgeGeometryTab(WingedEdge* mesh) {
    if (!mesh) {
        ImGui::Text("No mesh selected.");
        return;
    }

    // Retrieve the mesh name and ensure it exists.
    std::string meshName = meshCollection->getMeshName(mesh);
    if (meshName.empty()) {
        ImGui::Text("Mesh has no name assigned.");
        return;
    }

    // Retrieve the ObjectID and material once from the SceneManager.
    ObjectID objID = meshCollection->getObjectID(meshName);
    mat material = sceneManager->get(objID)->get_material();

    // Retrieve the cached center of the mesh.
    vec3 center = mesh->getCenter();
    ImGui::Text("Mesh Center: (%.2f, %.2f, %.2f)", center.x(), center.y(), center.z());
    ImGui::Separator();

    if (ImGui::BeginTabBar("TransformTabs")) {
        // Translate Tab
        if (ImGui::BeginTabItem("Translate")) {
            static float translation[3] = { 0.0f, 0.0f, 0.0f };
            ImGui::Text("Translate Mesh");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
            ImGui::SliderFloat3("Translation", translation, -2.0f, 2.0f);
            ImGui::PopItemWidth();

            if (ImGui::Button("Apply Translation")) {
                Matrix4x4 transform = Matrix4x4::translation(vec3(translation[0], translation[1], translation[2]));
                mesh->transform(transform);
                meshCollection->updateMeshRendering(*sceneManager, meshName, material);
                translation[0] = translation[1] = translation[2] = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Position")) {
                Matrix4x4 resetTransform = Matrix4x4::translation(vec3(-center.x(), -center.y(), -center.z()));
                mesh->transform(resetTransform);
                meshCollection->updateMeshRendering(*sceneManager, meshName, material);
            }
            ImGui::EndTabItem();
        }

        // Rotate Tab
        if (ImGui::BeginTabItem("Rotate")) {
            static float rotationAxis[3] = { 0.0f, 1.0f, 0.0f };
            static float rotationAngle = 0.0f;
            static bool useCustomRotationPoint = false;
            static float customRotationPoint[3] = { 0.0f, 0.0f, 0.0f };

            ImGui::Text("Rotate Mesh");
            ImGui::Checkbox("Use Custom Rotation Point", &useCustomRotationPoint);
            if (useCustomRotationPoint) {
                ImGui::Text("Custom Rotation Point:");
                ImGui::SliderFloat3("Rotation Point", customRotationPoint, -10.0f, 10.0f);
            }
            ImGui::SliderFloat3("Rotation Axis", rotationAxis, -1.0f, 1.0f);
            ImGui::SliderFloat("Angle (degrees)", &rotationAngle, -180.0f, 180.0f);

            if (ImGui::Button("Apply Rotation")) {
                vec3 axis(rotationAxis[0], rotationAxis[1], rotationAxis[2]);
                if (axis.length_squared() > 0.0f) {
                    axis = unit_vector(axis);
                    point3 rotationPoint = useCustomRotationPoint ?
                        point3(customRotationPoint[0], customRotationPoint[1], customRotationPoint[2]) :
                        center;
                    Matrix4x4 rotationMatrix = rotationMatrix.rotateAroundPoint(rotationPoint, axis, rotationAngle);
                    mesh->transform(rotationMatrix);
                    meshCollection->updateMeshRendering(*sceneManager, meshName, material);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Rotation")) {
                rotationAxis[0] = 0.0f; rotationAxis[1] = 1.0f; rotationAxis[2] = 0.0f;
                rotationAngle = 0.0f;
            }
            ImGui::EndTabItem();
        }

        // Scale Tab
        if (ImGui::BeginTabItem("Scale")) {
            static float scaleValues[3] = { 1.0f, 1.0f, 1.0f };
            static bool uniformScale = false;
            static float uniformScaleValue = 1.0f;

            ImGui::Text("Scale Mesh");
            ImGui::Checkbox("Uniform Scaling", &uniformScale);
            if (uniformScale) {
                ImGui::SliderFloat("Scale All Axes", &uniformScaleValue, 0.1f, 5.0f);
                scaleValues[0] = scaleValues[1] = scaleValues[2] = uniformScaleValue;
            }
            else {
                ImGui::SliderFloat3("Scale Axes", scaleValues, 0.1f, 5.0f);
            }
            if (ImGui::Button("Apply Scaling")) {
                Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-center.x(), -center.y(), -center.z()));
                Matrix4x4 scaleMatrix = Matrix4x4::scaling(scaleValues[0], scaleValues[1], scaleValues[2]);
                Matrix4x4 translateBack = Matrix4x4::translation(vec3(center.x(), center.y(), center.z()));
                Matrix4x4 finalTransform = translateBack * scaleMatrix * translateToOrigin;
                mesh->transform(finalTransform);
                meshCollection->updateMeshRendering(*sceneManager, meshName, material);
                scaleValues[0] = scaleValues[1] = scaleValues[2] = 1.0f;
                uniformScaleValue = 1.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Scaling")) {
                // Reset scaling functionality as needed.
            }
            ImGui::EndTabItem();
        }

        // Shear Tab
        if (ImGui::BeginTabItem("Shear")) {
            static float shearValues[3] = { 0.0f, 0.0f, 0.0f };
            static bool useCustomShearPoint = false;
            static float customShearPoint[3] = { 0.0f, 0.0f, 0.0f };

            ImGui::Text("Shear Mesh");
            ImGui::Checkbox("Use Custom Shearing Point", &useCustomShearPoint);
            if (useCustomShearPoint) {
                ImGui::Text("Custom Shearing Point:");
                ImGui::SliderFloat3("Shearing Point", customShearPoint, -10.0f, 10.0f);
            }
            ImGui::SliderFloat3("Shear Factors", shearValues, -1.0f, 1.0f);

            if (ImGui::Button("Apply Shear")) {
                point3 shearingPoint = useCustomShearPoint ?
                    point3(customShearPoint[0], customShearPoint[1], customShearPoint[2]) :
                    center;
                Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-shearingPoint.x(), -shearingPoint.y(), -shearingPoint.z()));
                Matrix4x4 shearMatrix = Matrix4x4::shearing(shearValues[0], shearValues[1], shearValues[2]);
                Matrix4x4 translateBack = Matrix4x4::translation(vec3(shearingPoint.x(), shearingPoint.y(), shearingPoint.z()));
                Matrix4x4 finalTransform = translateBack * shearMatrix * translateToOrigin;
                mesh->transform(finalTransform);
                meshCollection->updateMeshRendering(*sceneManager, meshName, material);
                shearValues[0] = shearValues[1] = shearValues[2] = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Shear")) {
                // Reset shear functionality as needed.
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void WingedEdgeImGui::animateCreateBox()
{
    if (!meshCollection) return;

    // Get the mesh (it should be the first and only one during animation).
    WingedEdge* mesh = meshCollection->getMesh(0); // Get the current (animated) mesh.
    if (!mesh) {
        animationState = AnimationState::Idle; // Stop if no mesh.
        return;
    }
    // Define vertices positions
    vec3 vmin(0.0, 0.0, 0.0); // Example values
    vec3 vmax(1.0, 1.0, 1.0); // Example values
    //Check if time elapsed is enough
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - animationStartTime);

    //Check if is enough time to run a new step
    if (elapsedTime.count() < animationStepDelay) {
        return; // Not enough time yet.
    }
    //Update timer
    animationStartTime = currentTime;

    switch (animationStep) {
    case 0: // Create v0
        mesh->MEV(nullptr, vec3(vmin.x(), vmin.y(), vmin.z()));
        break;
    case 1: // Create v1
        mesh->MEV(mesh->vertices[0].get(), vec3(vmax.x(), vmin.y(), vmin.z()));
        break;
    case 2: // Create v2
        mesh->MEV(mesh->vertices[1].get(), vec3(vmax.x(), vmax.y(), vmin.z()));
        break;
    case 3: // Create v3
        mesh->MEV(mesh->vertices[2].get(), vec3(vmin.x(), vmax.y(), vmin.z()));
        break;
    case 4: // Front face, part 1: v0, v1, v2
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[1].get(), mesh->vertices[2].get());
        break;
    case 5:  // Front face, part 2: v0, v2, v3
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[2].get(), mesh->vertices[3].get());
        break;
    case 6: // Create v4
        mesh->MEV(mesh->vertices[0].get(), vec3(vmin.x(), vmin.y(), vmax.z()));
        break;
    case 7: // Create v5
        mesh->MEV(mesh->vertices[1].get(), vec3(vmax.x(), vmin.y(), vmax.z()));
        break;
    case 8: // Create v6
        mesh->MEV(mesh->vertices[2].get(), vec3(vmax.x(), vmax.y(), vmax.z()));
        break;
    case 9: // Create v7
        mesh->MEV(mesh->vertices[3].get(), vec3(vmin.x(), vmax.y(), vmax.z()));
        break;
    case 10: // Back face, part 1: v4, v6, v5  (Reversed)
        mesh->MEF(mesh->vertices[4].get(), mesh->vertices[6].get(), mesh->vertices[5].get());
        break;
    case 11: // Back face, part 2: v4, v7, v6  (Reversed)
        mesh->MEF(mesh->vertices[4].get(), mesh->vertices[7].get(), mesh->vertices[6].get());
        break;
    case 12: // Bottom face, part 1: v0, v5, v1 (Reversed)
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[5].get(), mesh->vertices[1].get());
        break;
    case 13: // Bottom face, part 2: v0, v4, v5 (Reversed)
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[4].get(), mesh->vertices[5].get());
        break;
    case 14: // Top face, part 1: v3, v2, v6
        mesh->MEF(mesh->vertices[3].get(), mesh->vertices[2].get(), mesh->vertices[6].get());
        break;
    case 15: // Top face, part 2: v3, v6, v7
        mesh->MEF(mesh->vertices[3].get(), mesh->vertices[6].get(), mesh->vertices[7].get());
        break;
    case 16: // Left face, part 1: v0, v3, v7
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[3].get(), mesh->vertices[7].get());
        break;
    case 17: // Left face, part 2: v0, v7, v4
        mesh->MEF(mesh->vertices[0].get(), mesh->vertices[7].get(), mesh->vertices[4].get());
        break;
    case 18: // Right face, part 1: v1, v6, v2 (Reversed)
        mesh->MEF(mesh->vertices[1].get(), mesh->vertices[6].get(), mesh->vertices[2].get());
        break;
    case 19: // Right face, part 2: v1, v5, v6 (Reversed)
        mesh->MEF(mesh->vertices[1].get(), mesh->vertices[5].get(), mesh->vertices[6].get());
        break;

    default:
        animationState = AnimationState::Idle; // Animation finished
        break;
    }
    animationStep++;
}