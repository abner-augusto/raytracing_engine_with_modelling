#pragma once

#include "hittable.h"

struct CSGIntersection {
    double t;
    bool is_entry;
    const hittable* obj;
    vec3 normal;
    point3 p;

    CSGIntersection(double _t, bool _is_entry, const hittable* _obj,
        const vec3& _normal, const point3& _p)
        : t(_t), is_entry(_is_entry), obj(_obj),
        normal(_normal), p(_p) {
    }
};