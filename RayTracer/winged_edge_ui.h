#ifndef WINGED_EDGE_UI_H
#define WINGED_EDGE_UI_H

#include "imgui.h"
#include "winged_edge.h"
#include <string>
#include <vector>
#include <chrono>
#include <thread>


enum class AnimationState {
    Idle,
    Playing,
};

/**
 * @brief Handles the ImGui interface for exploring and interacting with a WingedEdge mesh.
 */
class WingedEdgeImGui {
private:
    MeshCollection* meshCollection;  ///< Pointer to the mesh collection being managed.
    SceneManager* sceneManager;
    size_t selectedMeshIndex = static_cast<size_t>(-1); ///< Index of the selected mesh.
    int selectedVertexIndex = -1;  ///< Index of the selected vertex.
    int selectedEdgeIndex = -1;    ///< Index of the selected edge.
    int selectedFaceIndex = -1;    ///< Index of the selected face.
    bool showEdgeDetails = false;  ///< Flag to show/hide edge details.
    bool showFaceDetails = false;  ///< Flag to show/hide face details.
    AnimationState animationState;
    int animationStep;
    std::chrono::steady_clock::time_point animationStartTime;
    int animationStepDelay = 250;


    //--------------------------------------------------------------------------
    // Helper Methods
    //--------------------------------------------------------------------------

    /**
     * @brief Returns a formatted string representation of an edge.
     * @param edge Pointer to the edge.
     * @return Formatted string for displaying the edge.
     */
    std::string getEdgeDisplayString(const Edge* edge);

    /**
     * @brief Returns a formatted string representation of a face.
     * @param face Pointer to the face.
     * @return Formatted string for displaying the face.
     */
    std::string getFaceDisplayString(const Face* face);

    //--------------------------------------------------------------------------
    // Rendering Methods
    //--------------------------------------------------------------------------

    /**
     * @brief Renders the mesh list in the UI.
     */
    void renderMeshList();

    /**
     * @brief Renders the details of the selected mesh.
     */
    void renderMeshDetails();

    /**
     * @brief Renders the vertices tab.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void renderVerticesTab(const WingedEdge* mesh);

    /**
     * @brief Renders the edges tab.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void renderEdgesTab(WingedEdge* mesh);

    /**
     * @brief Renders the faces tab.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void renderFacesTab(const WingedEdge* mesh);

    /**
     * @brief Renders detailed information about a selected edge.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void renderEdgeDetails(const WingedEdge* mesh);

    /**
     * @brief Renders detailed information about a selected face.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void renderFaceDetails(const WingedEdge* mesh);

    void animateCreateBox();

public:
    std::vector<std::shared_ptr<Edge>> selectedEdgeLoopEdges; ///< Stores the currently selected edge loop.

    /**
     * @brief Constructs the ImGui interface for managing WingedEdge meshes.
     * @param collection Pointer to the MeshCollection being managed.
     */
    explicit WingedEdgeImGui(MeshCollection* collection, SceneManager* world);

    /**
     * @brief Renders the ImGui UI for the WingedEdge mesh explorer.
     */
    void render();

    /**
     * @brief Updates the selection based on an external event (e.g., SDL input).
     * @param selectedMesh Pointer to the selected WingedEdge mesh.
     * @param selectedEdge Shared pointer to the selected edge.
     */
    void updateSelection(WingedEdge* selectedMesh, std::shared_ptr<Edge> selectedEdge);

    /**
     * @brief Retrieves the currently selected edge.
     * @return Shared pointer to the selected edge, or nullptr if no edge is selected.
     */
    [[nodiscard]] std::shared_ptr<Edge> getSelectedEdge() const;

    /**
     * @brief Retrieves the currently selected edge loop.
     * @return Reference to the vector containing selected edges in the loop.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Edge>>& getSelectedEdgeLoop() const {
        return selectedEdgeLoopEdges;
    }

    /**
     * @brief Computes and sets the linked edge loop.
     * @param mesh Pointer to the selected WingedEdge mesh.
     * @param clockwiseOnly If true, restricts traversal to clockwise edges.
     */
    void selectLinkedEdgeLoop(WingedEdge* mesh, bool clockwiseOnly = false);

    /**
     * @brief Computes and sets the linked edge loop in a counterclockwise direction.
     * @param mesh Pointer to the selected WingedEdge mesh.
     */
    void selectLinkedEdgeLoopCounterclockwise(WingedEdge* mesh);

    void selectEdgeChainByVertex(WingedEdge* mesh);

    void ShowWingedEdgeGeometryTab(WingedEdge* mesh);
};

#endif // WINGED_EDGE_UI_H
