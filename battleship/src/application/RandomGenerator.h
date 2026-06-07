#pragma once

namespace battleship::application {

class IRandomGenerator {
public:
    virtual ~IRandomGenerator() = default;

    virtual int nextInt(int minInclusive, int maxInclusive) = 0;
};

}
