#ifndef OBJECTS_H
#define OBJECTS_H

#include "boundingbox.h"

// Base class for all primitives
class Primitive {
public:
    virtual ~Primitive() = default;

    // Method to test intersection with a bounding box
    // Returns 'w' (outside), 'b' (inside), or 'p' (partial)
    virtual char test_bb(const BoundingBox& bb) const = 0;
};

#endif //OBJECTS_H