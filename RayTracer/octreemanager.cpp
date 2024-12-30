#include "octreemanager.h"
#include <stdexcept>

void OctreeManager::SelectOctree(size_t index) {
    if (index >= octrees.size()) {
        throw std::out_of_range("Index out of range in SelectOctree");
    }
    selected_octree_index = static_cast<int>(index);

    // Deselect all other octrees
    for (size_t i = 0; i < octrees.size(); ++i) {
        octrees[i].is_selected = (i == index);
    }
}

void OctreeManager::ResetSelectedOctree() {
    if (selected_octree_index < 0 || selected_octree_index >= static_cast<int>(octrees.size())) {
        throw std::runtime_error("No selected octree to reset");
    }

    OctreeWrapper& selected = octrees[selected_octree_index];
    BoundingBox bb = selected.octree->bounding_box;

    // Reset the octree by creating a new one with the same bounding box
    selected.octree = std::make_shared<Octree>(bb, Node::EmptyNode());
}

OctreeManager::OctreeWrapper& OctreeManager::GetSelectedOctree() {
    if (selected_octree_index < 0 || selected_octree_index >= static_cast<int>(octrees.size())) {
        throw std::runtime_error("No octree selected or invalid selected index");
    }
    return octrees[selected_octree_index];
}