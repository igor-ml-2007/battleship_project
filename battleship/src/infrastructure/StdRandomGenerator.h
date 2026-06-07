#pragma once

#include <random>

#include "application/RandomGenerator.h"

namespace battleship::infrastructure {

class StdRandomGenerator final : public application::IRandomGenerator {
public:
    StdRandomGenerator();

    int nextInt(int minInclusive, int maxInclusive) override;

private:
    std::mt19937 rng_;
};

}  // namespace battleship::infrastructure
