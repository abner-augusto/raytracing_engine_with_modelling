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
    auto boundary = f->getBoundary();
    for (size_t i = 0; i < boundary.size(); i++) {
        result += std::format("e{}", boundary[i]->index);
        if (i < boundary.size() - 1)
            result += ", ";
    }
    vec3 normal = f->normal();
    result += std::format(" | Normal: ({:.2f},{:.2f},{:.2f})",
        normal.x(), normal.y(), normal.z());
    return result;
}

//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
WingedEdgeImGui::WingedEdgeImGui(MeshCollection* collection)
    : meshCollection(collection), selectedMeshIndex(-1),
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

    if (selectedMeshIndex >= 0 && selectedMeshIndex < meshCollection->getMeshCount()) {
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
    if (meshCollection->getMeshCount() > 0 && ImGui::Button("Remove Selected") && selectedMeshIndex >= 0) {
        meshCollection->removeMesh(selectedMeshIndex);
        if (selectedMeshIndex >= meshCollection->getMeshCount()) {
            selectedMeshIndex = meshCollection->getMeshCount() - 1;
        }
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
    WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
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

void WingedEdgeImGui::renderVerticesTab(WingedEdge* mesh) {
    ImGui::Text("Vertices (%zu)", mesh->vertices.size());
    ImGui::Separator();

    ImGui::BeginChild("VerticesList", ImVec2(0, 120), true);
    for (const auto& vertex : mesh->vertices) {
        std::string vertexStr = std::format("v{}: ({:.4f}, {:.4f}, {:.4f})",
            vertex->index, vertex->pos.x(), vertex->pos.y(), vertex->pos.z());

        if (ImGui::Selectable(vertexStr.c_str(), selectedVertexIndex == vertex->index)) {
            selectedVertexIndex = vertex->index;
        }
    }
    ImGui::EndChild();

    // Display details for the selected vertex.
    for (const auto& vertex : mesh->vertices) {
        if (vertex->index == selectedVertexIndex) {
            ImGui::Separator();
            ImGui::Text("Vertex Details:");

            // Fix: Use std::format properly before passing the string to ImGui::Text
            std::string positionText = std::format("Position: ({:.4f}, {:.4f}, {:.4f})",
                vertex->pos.x(), vertex->pos.y(), vertex->pos.z());
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
void WingedEdgeImGui::renderEdgesTab(WingedEdge* mesh) {
    ImGui::Text("Edges (%zu)", mesh->edges.size());
    ImGui::Separator();

    ImGui::BeginChild("EdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < mesh->edges.size(); i++) {
        const auto& e = mesh->edges[i];
        std::string edgeStr = getEdgeDisplayString(e.get());
        if (ImGui::Selectable(edgeStr.c_str(), selectedEdgeIndex == static_cast<int>(i))) {
            selectedEdgeIndex = static_cast<int>(i);
            selectedEdgeLoopEdges.clear();
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

//-------------------------------------------------------------------
// Faces Tab
//-------------------------------------------------------------------
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

//-------------------------------------------------------------------
// Edge Details Rendering
//-------------------------------------------------------------------
void WingedEdgeImGui::renderEdgeDetails(WingedEdge* mesh) {
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
        ImGui::Text("%s", getEdgeDisplayString(e->ccw_l_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255));
    if (ImGui::TreeNode("CCW Right Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->ccw_r_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255));
    if (ImGui::TreeNode("CW Left Edge")) {
        ImGui::PopStyleColor();
        ImGui::Text("%s", getEdgeDisplayString(e->cw_l_edge).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 150, 255));
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
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255));
    if (ImGui::TreeNode("Left Face")) {
        ImGui::PopStyleColor();
        Face* lf = e->l_face.expired() ? nullptr : e->l_face.lock().get();
        ImGui::Text("%s", getFaceDisplayString(lf).c_str());
        ImGui::TreePop();
    }
    else {
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 255, 150, 255));
    if (ImGui::TreeNode("Right Face")) {
        ImGui::PopStyleColor();
        Face* rf = e->r_face.expired() ? nullptr : e->r_face.lock().get();
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
void WingedEdgeImGui::renderFaceDetails(WingedEdge* mesh) {
    Face* f = mesh->faces[selectedFaceIndex].get();

    ImGui::Indent();
    vec3 normal = f->normal();
    ImGui::Text("Normal: (%.4f, %.4f, %.4f)", normal.x(), normal.y(), normal.z());
    ImGui::Separator();
    ImGui::Text("Boundary Edges:");
    auto boundary = f->getBoundary();
    ImGui::BeginChild("FaceEdgesList", ImVec2(0, 120), true);
    for (size_t i = 0; i < boundary.size(); i++) {
        Edge* e = boundary[i];
        std::string edgeStr = std::format("{}. {}", i + 1, getEdgeDisplayString(e));
        if (ImGui::Selectable(edgeStr.c_str())) {
            for (size_t j = 0; j < mesh->edges.size(); j++) {
                if (mesh->edges[j].get() == e) {
                    selectedEdgeIndex = static_cast<int>(j);
                    break;
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
    for (size_t i = 0; i < meshCollection->getMeshCount(); i++) {
        if (meshCollection->getMesh(i) == selectedMesh) {
            selectedMeshIndex = i;
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

std::shared_ptr<Edge> WingedEdgeImGui::getSelectedEdge() const {
    if (selectedMeshIndex < meshCollection->getMeshCount()) {
        WingedEdge* mesh = meshCollection->getMesh(selectedMeshIndex);
        if (selectedEdgeIndex >= 0 && selectedEdgeIndex < static_cast<int>(mesh->edges.size())) {
            return mesh->edges[selectedEdgeIndex];
        }
    }
    return nullptr;
}

void WingedEdgeImGui::selectLinkedEdgeLoop(WingedEdge* mesh, bool clockwiseOnly) {
    if (selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size()))
        return;

    selectedEdgeLoopEdges.clear();
    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    Edge* currentRaw = startEdge.get();
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;

    // Traverse clockwise using the cw_r_edge pointer.
    while (currentRaw->cw_r_edge && currentRaw->cw_r_edge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->cw_r_edge->index) > 0)
            break;
        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<Edge>& e) {
                return e.get() == currentRaw->cw_r_edge;
            });
        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->cw_r_edge;
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
        std::vector<std::shared_ptr<Edge>> reverseEdges;
        std::ostringstream reverseLogStream;
        while (currentRaw->ccw_l_edge && currentRaw->ccw_l_edge != startEdge.get()) {
            if (visitedEdges.count(currentRaw->ccw_l_edge->index) > 0)
                break;
            auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
                [currentRaw](const std::shared_ptr<Edge>& e) {
                    return e.get() == currentRaw->ccw_l_edge;
                });
            if (it != mesh->edges.end()) {
                reverseEdges.push_back(*it);
                visitedEdges.insert((*it)->index);
                currentRaw = currentRaw->ccw_l_edge;
                reverseLogStream << "e" << (*it)->index << "->";
            }
            else {
                break;
            }
        }
        std::reverse(reverseEdges.begin(), reverseEdges.end());
        selectedEdgeLoopEdges.insert(selectedEdgeLoopEdges.begin(), reverseEdges.begin(), reverseEdges.end());
        std::cout << "Edge Loop: " << reverseLogStream.str() << logStream.str() << std::endl;
    }
    else {
        std::cout << "Edge Loop (Clockwise Only): " << logStream.str() << std::endl;
    }
}

void WingedEdgeImGui::selectLinkedEdgeLoopCounterclockwise(WingedEdge* mesh) {
    if (selectedEdgeIndex < 0 || selectedEdgeIndex >= static_cast<int>(mesh->edges.size()))
        return;

    selectedEdgeLoopEdges.clear();
    auto startEdge = mesh->edges[selectedEdgeIndex];
    selectedEdgeLoopEdges.push_back(startEdge);

    std::unordered_set<int> visitedEdges;
    visitedEdges.insert(startEdge->index);

    Edge* currentRaw = startEdge.get();
    std::ostringstream logStream;
    logStream << "e" << startEdge->index;

    while (currentRaw->ccw_l_edge && currentRaw->ccw_l_edge != startEdge.get()) {
        if (visitedEdges.count(currentRaw->ccw_l_edge->index) > 0)
            break;
        auto it = std::find_if(mesh->edges.begin(), mesh->edges.end(),
            [currentRaw](const std::shared_ptr<Edge>& e) {
                return e.get() == currentRaw->ccw_l_edge;
            });
        if (it != mesh->edges.end()) {
            selectedEdgeLoopEdges.push_back(*it);
            visitedEdges.insert((*it)->index);
            currentRaw = currentRaw->ccw_l_edge;
            logStream << "->e" << (*it)->index;
        }
        else {
            break;
        }
    }
    std::cout << "Edge Loop (Counterclockwise Only): " << logStream.str() << std::endl;
}
