#ifndef WINGED_EDGE_UI_H
#define WINGED_EDGE_UI_H

#include "imgui.h"
#include "winged_edge.h"
#include <string>

class WingedEdgeImGui {
private:
    MeshCollection* meshCollection;
    int selectedMeshIndex = -1;
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
    WingedEdgeImGui(MeshCollection* collection);
    void render();
};

// Function to use in your main loop
void drawWingedEdgeImGui(MeshCollection& meshCollection);

#endif // WINGED_EDGE_UI_H