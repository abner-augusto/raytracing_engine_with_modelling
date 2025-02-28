#ifndef WINGED_EDGE_UI_H
#define WINGED_EDGE_UI_H

#include "imgui.h"
#include "winged_edge.h"
#include <string>

class WingedEdgeImGui {
private:
    MeshCollection* meshCollection;
    size_t selectedMeshIndex = -1;
    int selectedEdgeIndex = -1;
    int selectedFaceIndex = -1;
    bool showEdgeDetails = false;
    bool showFaceDetails = false;

    // Helper methods declarations
    std::string getEdgeDisplayString(const edge* e);
    std::string getFaceDisplayString(const face* f);

    // Private rendering methods
    void renderMeshList();
    void renderMeshDetails();
    void renderVerticesTab(WingedEdge* mesh);
    void renderEdgesTab(WingedEdge* mesh);
    void renderFacesTab(WingedEdge* mesh);
    void renderEdgeDetails();
    void renderFaceDetails();

public:
    std::vector<std::shared_ptr<edge>> selectedEdgeLoopEdges;
    WingedEdgeImGui(MeshCollection* collection);
    void render();
    // Update selection from another event (SDL)
    void updateSelection(WingedEdge* selectedMesh, std::shared_ptr<edge> selectedEdge);
    // Retrieve the currently selected edge (for use in SDL rendering).
    std::shared_ptr<edge> getSelectedEdge() const;
    // New getter for the edge loop selection
    const std::vector<std::shared_ptr<edge>>& getSelectedEdgeLoop() const {
        return selectedEdgeLoopEdges;
    }
    // New method to compute and set the edge loop
    void selectLinkedEdgeLoop(WingedEdge* mesh, bool clockwiseOnly = false);
    void selectLinkedEdgeLoopCounterclockwise(WingedEdge* mesh);
};

#endif // WINGED_EDGE_UI_H