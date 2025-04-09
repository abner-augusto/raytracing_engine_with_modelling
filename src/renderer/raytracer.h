#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <algorithm>
#include <random>

// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const float pi = 3.1415926535897932386f;

// Common Headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"
#include "vec4.h"
#include "matrix4x4.h"
#include "hittable.h"
#include "scene.h"

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double radians_to_degrees(double radians) {
    return radians * 180.0 / pi;
}

inline double random_double(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

inline point3 random_position() {
    return point3(
        random_double(-2.0, 2.0),  // x
        -0.15,                     // y
        random_double(-3.5, -1.0)  // z
    );
}

// Helper function that duplicates an existing object in the SceneManager
// and translates each copy along a specified direction.
auto static duplicateObjectArray(SceneManager& world, ObjectID originalID, int numCopies, float fixedDistance, const vec3& direction, bool applyRotation = false) {
    if (!world.contains(originalID)) {
        std::cerr << "Error: Original ObjectID " << originalID << " not found in SceneManager.\n";
        return;
    }

    // Retrieve the original object
    auto originalObject = world.get(originalID);
    if (!originalObject) {
        std::cerr << "Error: Failed to retrieve object with ID " << originalID << ".\n";
        return;
    }

    for (int i = 0; i < numCopies; i++) {
        // Clone the original object
        std::shared_ptr<hittable> newObject = originalObject->clone();
        if (!newObject) {
            std::cerr << "Error: Cloning failed for object ID " << originalID << ".\n";
            return;
        }

        // Add the new instance to the scene
        ObjectID newID = world.add(newObject);

        // Calculate translation offset
        vec3 offset = direction * (fixedDistance * (i + 1));
        Matrix4x4 translate = Matrix4x4::translation(offset);

        // Apply optional rotation
        Matrix4x4 rotate;
        if (applyRotation) {
            vec3 rotateDirection = vec3(0, 1, 0); // Example: Y-axis rotation
            float angle = 10.0f; // Example: Rotation angle increment
            rotate = rotate.rotateAroundPoint(world.get(newID)->bounding_box().getCenter(), rotateDirection, angle * (i + 5));
        }
        else {
            Matrix4x4 rotate;
        }

        Matrix4x4 transform = translate * rotate;

        // Apply transformation to the new instance
        world.transform_object(newID, transform);
    }
}

#endif