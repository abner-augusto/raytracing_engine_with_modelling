#include "winged_edge_ui.h"
#include <format>
#include <vector>
#include <memory>
#include <algorithm>

// Helper methods implementation
std::string WingedEdgeImGui::getEdgeDisplayString(const edge* e) {
    if (!e) return "nullptr";
    return std::format("e{}: ({:.2f},{:.2f},{:.2f}) -> ({:.2f},{:.2f},{:.2f})",
        e->index,
        e->origVec.x(), e->origVec.y(), e->origVec.z(),
        e->destVec.x(), e->destVec.y(), e->destVec.z());
}

std::string WingedEdgeImGui::getFaceDisplayString(const face* f) {
    if (!f) return "nullptr";

    std::string result = std::format("f{}: ", f->index);
    for (size_t i = 0; i < f->vEdges.size(); i++) {
        result += std::format("e{}{}",
            f->vEdges[i]->index,
            f->vIsReversed[i] ? "(R)" : "");

        if (i < f->vEdges.size() - 1) {
            result += ", ";
        }
    }

    vec3 normal = f->normal();
    result += std::format(" | Normal: ({:.2f},{:.2f},{:.2f})",
        normal.x(), normal.y(), normal.z());

    return result;
}

// Constructor
WingedEdgeImGui::WingedEdgeImGui(MeshCollection* collection) : meshCollection(collection) {}

// Main render method
void WingedEdgeImGui::render() {
    if (!meshCollection) return;

    // Main window for the mesh collection
    ImGui::Begin("WingedEdge Mesh Explorer");
    renderMeshList();
    ImGui::End();

    // Show the selected mesh details if a mesh is selected
    if (selectedMeshIndex >= 0 && selectedMeshIndex < meshCollection->getMeshCount()) {
        renderMeshDetails();
    }

}

void WingedEdgeImGui::renderMeshList() {
    ImGui::Text("Mesh Collection (%zu meshes)", meshCollection->getMeshCount());
    ImGui::Separator();

    if (meshCollection->getMeshCount() == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No meshes in collection");
        return;
    }

    ImGui::BeginChild("MeshList", ImVec2(0, 150), true);
    for (size_t i = 0; i < meshCollection->getMeshCount(); i++) {
        const WingedEdge* mesh = meshCollection->getMesh(i);

        // Get the mesh name dynamically
        std::string meshName = meshCollection->getMeshName(mesh);

        // If the mesh has no name, use a default identifier
        if (meshName.empty()) {
            meshName = "Mesh " + std::to_string(i);
        }

        // Append mesh details
        meshName += std::format(" - V:{} E:{} F:{}",
            mesh->vertices.size(), mesh->edges.size(), mesh->faces.size());

        // Render the selectable item
        if (ImGui::Selectable(meshName.c_str(), selectedMeshIndex == static_cast<int>(i))) {
            selectedMeshIndex = static_cast<int>(i);
            selectedEdgeIndex = -1;  // Reset selected edge
            selectedFaceIndex = -1;  // Reset selected face
            showEdgeDetails = false;
            showFaceDetails = false;
        }
    }
    ImGui::EndChild();


    if (ImGui::Button("Add Tetrahedron")) {
        auto newMesh = PrimitiveFactory::createTetrahedron();
        meshCollection->addMesh(std::move(newMesh), "Tetrahedron_" + std::to_string(meshCollection->getMeshCount()));
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove Selected") && selectedMeshIndex >= 0) {
        meshCollection->removeMesh(selectedMeshIndex);
        if (selectedMeshIndex >= meshCollection->getMeshCount()) {
            selectedMeshIndex = meshCollection->getMeshCount() - 1;
        }
        selectedEdgeIndex = -1;
        selectedFaceIndex = -1;
        showEdgeDetails = false;
        showFaceDetails = false;
    }
}

void WingedEdgeImGui::renderMeshDetails() {
    WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);

    std::string title = "Mesh Details";

    // Get the name of the selected mesh
    std::string meshName = meshCollection->getMeshName(mesh);

    if (!meshName.empty()) {
        title += " - " + meshName;
    }

    ImGui::Begin(title.c_str());

    // Tabs for different mesh components
    if (ImGui::BeginTabBar("MeshTabs")) {
        if (ImGui::BeginTabItem("Vertices")) {
            renderVerticesTab(mesh);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Edges")) {
            renderEdgesTab(mesh);

            // Add a separator after the loop buttons
            ImGui::Separator();

            // Render edge details as a collapsible header if an edge is selected
            if (selectedEdgeIndex >= 0 && selectedEdgeIndex < mesh->edges.size()) {
                edge* e = mesh->edges[selectedEdgeIndex].get();
                std::string headerText = std::format("Edge Details (e{})", e->index);

                // Use a color for the collapsing header to make it stand out
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

            // Add a separator after the faces list
            ImGui::Separator();

            // Render face details as a collapsible header if a face is selected
            if (selectedFaceIndex >= 0 && selectedFaceIndex < mesh->faces.size()) {
                face* f = mesh->faces[selectedFaceIndex].get();
                std::string headerText = std::format("Face Details (f{})", f->index);

                // Use a color for the collapsing header to make it stand out
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

void WingedEdgeImGui::renderVerticesTab(WingedEdge* mesh) {
    ImGui::Text("Vertices (%zu)", mesh->vertices.size());
    ImGui::Separator();

    ImGui::BeginChild("VerticesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->vertices.size(); i++) {
        const vec3& v = mesh->vertices[i];
        std::string vertexStr = std::format("v{}: ({:.4f}, {:.4f}, {:.4f})",
            i, v.x(), v.y(), v.z());

        if (ImGui::Selectable(vertexStr.c_str())) {
            // If needed, add vertex selection handling
        }
    }
    ImGui::EndChild();
}

void WingedEdgeImGui::renderEdgesTab(WingedEdge* mesh) {
    ImGui::Text("Edges (%zu)", mesh->edges.size());
    ImGui::Separator();

    ImGui::BeginChild("EdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->edges.size(); i++) {
        const auto& e = mesh->edges[i];
        std::string edgeStr = getEdgeDisplayString(e.get());

        if (ImGui::Selectable(edgeStr.c_str(), selectedEdgeIndex == static_cast<int>(i))) {
            selectedEdgeIndex = static_cast<int>(i);
            // When manually selecting an edge, clear the loop selection
            selectedEdgeLoopEdges.clear();
            // Add the newly selected edge to the set
            selectedEdgeLoopEdges.push_back(mesh->edges[selectedEdgeIndex]);
            showEdgeDetails = true;
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Full Edge Loop")) {
        selectLinkedEdgeLoop(mesh, false);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clockwise")) {
        selectLinkedEdgeLoop(mesh, true);
    }

    ImGui::SameLine();

    if (ImGui::Button("Counterclockwise")) {
        selectLinkedEdgeLoopCounterclockwise(mesh);
    }
}

void WingedEdgeImGui::renderFacesTab(WingedEdge* mesh) {
    ImGui::Text("Faces (%zu)", mesh->faces.size());
    ImGui::Separator();

    ImGui::BeginChild("FacesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->faces.size(); i++) {
        const auto& f = mesh->faces[i];
        std::string faceStr = getFaceDisplayString(f.get());

        if (ImGui::Selectable(faceStr.c_str(), selectedFaceIndex == static_cast<int>(i))) {
            selectedFaceIndex = static_cast<int>(i);
            showFaceDetails = true;
        }
    }
    ImGui::EndChild();
}

void WingedEdgeImGui::renderEdgeDetails(WingedEdge* mesh) {
    edge* e = mesh->edges[selectedEdgeIndex].get();

    ImGui::Indent();  // Add indentation for better visual hierarchy

    // Use color for basic properties
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "Origin:");
    ImGui::SameLine();
    ImGui::Text("(%.4f, %.4f, %.4f)", e->origVec.x(), e->origVec.y(), e->origVec.z());

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "Destination:");
    ImGui::SameLine();
    ImGui::Text("(%.4f, %.4f, %.4f)", e->destVec.x(), e->destVec.y(), e->destVec.z());

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Wing Pointers:");

    // Display wing pointers with improved color contrast
    // Use different colors for different types of wing pointers
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255)); // Light blue for CCW
    if (ImGui::TreeNode("CCW Left Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->ccw_l_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255)); // Light blue for CCW
    if (ImGui::TreeNode("CCW Right Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->ccw_r_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255)); // Light orange for CW
    if (ImGui::TreeNode("CW Left Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->cw_l_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255)); // Light orange for CW
    if (ImGui::TreeNode("CW Right Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->cw_r_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Adjacent Faces:");

    // Display adjacent faces with green color theme
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255)); // Light green
    if (ImGui::TreeNode("Left Face")) {
        ImGui::PopStyleColor();
        face* lf = e->l_face.expired() ? nullptr : e->l_face.lock().get();
        ImGui::Text("%s", getFaceDisplayString(lf).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255)); // Light green
    if (ImGui::TreeNode("Right Face")) {
        ImGui::PopStyleColor();
        face* rf = e->r_face.expired() ? nullptr : e->r_face.lock().get();
        ImGui::Text("%s", getFaceDisplayString(rf).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::Unindent();  // Remove indentation
}

void WingedEdgeImGui::renderFaceDetails(WingedEdge* mesh) {
    face* f = mesh->faces[selectedFaceIndex].get();

    ImGui::Indent();  // Add indentation for better visual hierarchy

    if (f->bIsTetra) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Tetrahedron Face");
    }

    vec3 normal = f->normal();
    ImGui::Text("Normal: (%.4f, %.4f, %.4f)", normal.x(), normal.y(), normal.z());

    ImGui::Separator();
    ImGui::Text("Edges (%zu):", f->vEdges.size());

    ImGui::BeginChild("FaceEdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < f->vEdges.size(); i++) {
        edge* e = f->vEdges[i];
        bool reversed = f->vIsReversed[i];

        std::string edgeStr = std::format("{}. {}{}",
            i + 1, getEdgeDisplayString(e), reversed ? " (Reversed)" : "");

        if (ImGui::Selectable(edgeStr.c_str())) {
            // Find the edge index in the mesh
            for (size_t j = 0; j < mesh->edges.size(); j++) {
                if (mesh->edges[j].get() == e) {
                    selectedEdgeIndex = static_cast<int>(j);
                    break;
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::Unindent();  // Remove indentation
}

// --- Added method to update the selection ---
void WingedEdgeImGui::updateSelection(WingedEdge* selectedMesh, std::shared_ptr<edge> selectedEdge) {
    if (!selectedMesh || !selectedEdge)
        return;
    // Find the mesh index within the mesh collection.
    for (size_t i = 0; i < meshCollection->getMeshCount(); i++) {
        if (meshCollection->getMesh(i) == selectedMesh) {
            selectedMeshIndex = i;
            // Now, search for the edge index within the mesh.
            for (size_t j = 0; j < selectedMesh->edges.size(); j++) {
                if (selectedMesh->edges[j] == selectedEdge) {
                    selectedEdgeIndex = static_cast<int>(j);
                    break;
                }
            }
            break;
        }
    }
}

// --- Added method to retrieve the currently selected edge ---
std::shared_ptr<edge> WingedEdgeImGui::getSelectedEdge() const {
    if (selectedMeshIndex < meshCollection->getMeshCount()) {
        WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
        if (selectedEdgeIndex >= 0 && selectedEdgeIndex < static_cast<int>(mesh->edges.size())) {
            return mesh->edges[selectedEdgeIndex];
        }
    }
    return nullptr;
}

void WingedEdgeImGui::selectLinkedEdgeLoop(WingedEdge* mesh, bool clockwiseOnly) {
    if (selectedEdgeIndex < 0 || selectedEdgeIndex >= mesh->edges.size())
        return;

    selectedEdgeLoopEdges.clear();

    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    edge* currentRaw = startEdge.get();
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;  // Start the log

    // Traverse clockwise
    while (currentRaw->cw_r_edge && currentRaw->cw_r_edge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->cw_r_edge->index) > 0)
            break;

        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<edge>& e) {
                return e.get() == currentRaw->cw_r_edge;
            });

        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->cw_r_edge;

            // Append to the log
            logStream << "->e" << (*it)->index;
        }
        else {
            break;
        }
    }

    if (!clockwiseOnly) {
        visitedEdges.clear();
        visitedEdges.insert(startEdge->index);

        currentRaw = startEdge.get();
        std::vector<std::shared_ptr<edge>> reverseEdges;
        std::ostringstream reverseLogStream;  // Separate log for counterclockwise

        while (currentRaw->ccw_l_edge && currentRaw->ccw_l_edge != startEdge.get()) {
            if (visitedEdges.count(currentRaw->ccw_l_edge->index) > 0)
                break;

            auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
                [currentRaw](const std::shared_ptr<edge>& e) {
                    return e.get() == currentRaw->ccw_l_edge;
                });

            if (it != mesh->edges.end()) {
                reverseEdges.push_back(*it);
                visitedEdges.insert((*it)->index);
                currentRaw = currentRaw->ccw_l_edge;

                // Append to the reverse log
                reverseLogStream << "e" << (*it)->index << "->";
            }
            else {
                break;
            }
        }

        std::reverse(reverseEdges.begin(), reverseEdges.end());
        selectedEdgeLoopEdges.insert(selectedEdgeLoopEdges.begin(), reverseEdges.begin(), reverseEdges.end());

        // Print the counterclockwise log first, then the clockwise log
        std::cout << "Edge Loop: " << reverseLogStream.str() << logStream.str() << std::endl;
    }
    else {
        std::cout << "Edge Loop (Clockwise Only): " << logStream.str() << std::endl;
    }
}

void WingedEdgeImGui::selectLinkedEdgeLoopCounterclockwise(WingedEdge* mesh) {
    if (selectedEdgeIndex < 0 || selectedEdgeIndex >= mesh->edges.size())
        return;

    selectedEdgeLoopEdges.clear();

    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    edge* currentRaw = startEdge.get();
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;  // Start the log

    while (currentRaw->ccw_l_edge && currentRaw->ccw_l_edge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->ccw_l_edge->index) > 0)
            break;

        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<edge>& e) {
                return e.get() == currentRaw->ccw_l_edge;
            });

        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->ccw_l_edge;

            // Append to the log
            logStream << "->e" << (*it)->index;
        }
        else {
            break;
        }
    }

    std::cout << "Edge Loop (Counterclockwise Only): " << logStream.str() << std::endl;
}
