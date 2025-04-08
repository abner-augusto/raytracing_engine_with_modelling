#ifndef INTERVAL_H
#define INTERVAL_H
#include <limits>

class interval {
public:
    double min, max;

    // Default constructor - creates empty interval
    interval() : min(+std::numeric_limits<double>::infinity()),
        max(-std::numeric_limits<double>::infinity()) {
    }

    // Constructor with min and max
    interval(double _min, double _max) : min(_min), max(_max) {}

    // Returns interval size
    double size() const {
        return max - min;
    }

    // Check if x is within the interval
    bool contains(double x) const {
        return min <= x && x <= max;
    }

    // Check if x is within the interval with bias
    bool contains(double x, double bias) const {
        return (min - bias) <= x && x <= (max + bias);
    }

    // Check if x is strictly within the interval
    bool surrounds(double x) const {
        return min < x && x < max;
    }

    // Check if x is strictly within the interval with bias
    bool surrounds(double x, double bias) const {
        return (min - bias) < x && x < (max + bias);
    }

    // Clamp a value to the interval
    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    // Create a new interval with bias
    interval with_bias(double bias) const {
        return interval(min - bias, max + bias);
    }

    // Expand the interval by a bias amount
    void expand(double bias) {
        min -= bias;
        max += bias;
    }

    static const interval empty;
    static const interval universe;
};

#endif