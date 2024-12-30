#ifndef OCTREEMANAGER_H
#define OCTREEMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include "octree.h"
#include "hittable_list.h"
#include "box.h"

class OctreeManager {
public:
    struct OctreeWrapper {
        std::string name;
        std::shared_ptr<Octree> octree;
        bool is_selected = false;

        OctreeWrapper(const std::string& n, const BoundingBox& bb)
            : name(n), octree(std::make_shared<Octree>(bb, Node::EmptyNode())) {
        }
    };

    int depth_limit = 3;
    const std::vector<OctreeWrapper>& GetOctrees() const { return octrees; }
    std::vector<OctreeWrapper>& GetOctrees() { return octrees; }
    OctreeWrapper& GetSelectedOctree();
    int GetSelectedIndex() const { return selected_octree_index; }
    void SelectOctree(size_t index);
    void ResetSelectedOctree();

    OctreeManager() = default;

    // Adds a new octree with a sequentially numbered name
    void AddOctree(const std::string& base_name, const BoundingBox& bounding_box) {
        std::string unique_name = base_name + " " + std::to_string(octree_counter++);
        octrees.emplace_back(unique_name, bounding_box);
    }

    // Removes the currently selected octree
    void RemoveSelectedOctree() {
        if (selected_octree_index >= 0 && selected_octree_index < static_cast<int>(octrees.size())) {
            octrees.erase(octrees.begin() + selected_octree_index);
            selected_octree_index = -1; // Clear selection after removal
        }
    }

    void SetName(int index, const std::string& new_name) {
        if (index < 0 || index >= static_cast<int>(octrees.size())) {
            throw std::out_of_range("Invalid octree index");
        }
        octrees[index].name = new_name;
    }

    // Add filled bounding boxes from a specific octree to the world
    static void RenderFilledBBs(const OctreeManager& manager, size_t index, hittable_list& world, mat& material, bool use_random_colors = false) {
        if (index >= manager.GetOctrees().size()) {
            throw std::out_of_range("Invalid octree index");
        }

        auto filled_boxes = manager.GetOctrees()[index].octree->GetFilledBoundingBoxes();
        for (const auto& bb : filled_boxes) {
            mat current_material = use_random_colors ? mat(random_color()) : material;
            world.add(std::make_shared<box>(bb.vmin, bb.vmax(), current_material));
        }
    }

    // Remove filled bounding boxes of a specific octree from the world
    static void RemoveFilledBBs(const Octree& octree, hittable_list& world) {
        // Retrieve the filled bounding boxes for the specific octree
        auto filled_boxes = octree.GetFilledBoundingBoxes();

        // Iterate over the filled bounding boxes
        for (const auto& bb : filled_boxes) {
            // Remove objects matching the bounding box dimensions
            world.remove_if([&](const std::shared_ptr<hittable>& obj) {
                auto box_obj = dynamic_cast<box*>(obj.get());
                return box_obj != nullptr &&
                    box_obj->min_corner == bb.vmin &&
                    box_obj->v_max == bb.vmax();
                });
        }
    }

    void RebuildFromAnother(size_t sourceIndex, int maxDepth) {
        if (selected_octree_index < 0 || selected_octree_index >= static_cast<int>(octrees.size())) {
            throw std::out_of_range("No valid octree is selected for rebuilding");
        }
        if (sourceIndex >= octrees.size()) {
            throw std::out_of_range("Invalid source octree index");
        }

        // Get the source octree's bounding box
        const BoundingBox& newBB = octrees[sourceIndex].octree->bounding_box;

        // Get the selected octree
        auto& selectedOctree = octrees[selected_octree_index];

        // Rebuild the selected octree using the bounding box of the source octree
        selectedOctree.octree = std::make_shared<Octree>(
            Octree::RebuildOctreeFromBbs(*selectedOctree.octree, newBB, maxDepth)
        );
    }


    void PerformBooleanOperation(int index1, int index2, const std::string& operation, int depth_limit)
    {
        // Validate indices
        if (index1 < 0 || index1 >= static_cast<int>(octrees.size()) ||
            index2 < 0 || index2 >= static_cast<int>(octrees.size()))
        {
            return;
        }

        const Octree& octree1 = *octrees[index1].octree;
        const Octree& octree2 = *octrees[index2].octree;

        // Check if the bounding boxes are already aligned
        if (octree1.bounding_box == octree2.bounding_box) {
            // Add a new empty octree to store the result
            this->AddOctree("Result " + operation, octree1.bounding_box);

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

private:
    std::vector<OctreeWrapper> octrees;
    int selected_octree_index = -1;
    int octree_counter = 1; // Tracks the number of octrees created
};

#endif // OCTREEMANAGER_H
