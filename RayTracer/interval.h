#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits> // Necessário para std::numeric_limits

class interval {
public:
    double min, max;

    interval() : min(+std::numeric_limits<double>::infinity()), max(-std::numeric_limits<double>::infinity()) {} // Intervalo padrão é vazio
    interval(double min, double max) : min(min), max(max) {}

    double size() const {
        return max - min;
    }

    bool contains(double x) const {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    static const interval empty;
    static const interval universe;
};

#endif
