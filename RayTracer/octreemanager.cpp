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

void OctreeManager::PerformBooleanOperation(int index1, int index2, const std::string& operation, int depth_limit)
{
    // Validate indices
    if (index1 < 0 || index1 >= static_cast<int>(octrees.size()) ||
        index2 < 0 || index2 >= static_cast<int>(octrees.size()))
    {
        // Invalid indices - just return or throw
        return;
    }

    const Octree& octree1 = *octrees[index1].octree;
    const Octree& octree2 = *octrees[index2].octree;

    // Check if the bounding boxes are already aligned
    if (octree1.bounding_box == octree2.bounding_box) {
        // Add a new empty octree to store the result
        this->AddOctree("Result "+ operation, octree1.bounding_box);

        // The newly added octree will be the last one in octrees
        auto& result_wrapper = octrees.back();
        Octree& result_octree = *result_wrapper.octree;

        // Perform the boolean operation directly
        result_octree.root = Node::BooleanRecursive(
            octree1.root,
            octree2.root,
            operation
        );
    }
    else {
        // Compute a bounding box that encloses both
        BoundingBox merged_bb = octree1.bounding_box.Enclose(octree2.bounding_box);

        // Add a new empty octree to store the result
        this->AddOctree("Result " + operation, merged_bb);

        // The newly added octree will be the last one in octrees
        auto& result_wrapper = octrees.back();
        Octree& result_octree = *result_wrapper.octree;

        // Rebuild both octrees to align them with the merged bounding box
        Octree rebuilt1 = Octree::RebuildOctreeFromBbs(octree1, merged_bb, depth_limit);
        Octree rebuilt2 = Octree::RebuildOctreeFromBbs(octree2, merged_bb, depth_limit);

        // Perform the boolean operation on the rebuilt octrees
        result_octree.root = Node::BooleanRecursive(
            rebuilt1.root,
            rebuilt2.root,
            operation
        );
    }
}

