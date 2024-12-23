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

    OctreeManager() = default;

    // Adds a new octree with a sequentially numbered name
    void AddOctree(const std::string& base_name, const BoundingBox& bounding_box) {
        std::string unique_name = base_name + " " + std::to_string(octree_counter++);
        octrees.emplace_back(unique_name, bounding_box);
    }

    void SetName(int index, const std::string& new_name) {
        if (index < 0 || index >= static_cast<int>(octrees.size())) {
            throw std::out_of_range("Invalid octree index");
        }
        octrees[index].name = new_name;
    }

    const std::vector<OctreeWrapper>& GetOctrees() const { return octrees; }
    std::vector<OctreeWrapper>& GetOctrees() { return octrees; }
    OctreeWrapper& GetSelectedOctree();
    int GetSelectedIndex() const { return selected_octree_index; }
    void SelectOctree(size_t index);
    void ResetSelectedOctree();

    // Add filled bounding boxes from a specific octree to the world
    static void RenderFilledBBs(const OctreeManager& manager, size_t index, hittable_list& world) {
        if (index >= manager.GetOctrees().size()) {
            throw std::out_of_range("Invalid octree index");
        }
        auto filled_boxes = manager.GetOctrees()[index].octree->GetFilledBoundingBoxes();
        for (const auto& bb : filled_boxes) {
            world.add(std::make_shared<box>(bb.vmin, bb.vmax(), mat(random_color())));
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

private:
    std::vector<OctreeWrapper> octrees;
    int selected_octree_index = -1;
    int octree_counter = 1; // Tracks the number of octrees created
};

#endif // OCTREEMANAGER_H
