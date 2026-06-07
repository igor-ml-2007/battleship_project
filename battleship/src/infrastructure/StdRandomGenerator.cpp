#include "infrastructure/StdRandomGenerator.h"

#include <chrono>

namespace battleship::infrastructure {

StdRandomGenerator::StdRandomGenerator()
    : rng_(static_cast<std::mt19937::result_type>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {
}

int StdRandomGenerator::nextInt(int minInclusive, int maxInclusive) {
    std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
    return distribution(rng_);
}

}  // namespace battleship::infrastructure
