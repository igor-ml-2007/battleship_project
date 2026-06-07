#pragma once

#include <vector>

#include "application/RandomGenerator.h"
#include "domain/AiStrategy.h"
#include "domain/Types.h"

namespace battleship::domain {

class AIPlayer final : public IAiStrategy {
public:
    AIPlayer();

    std::string name() const override;
    void reset() override;
    Point chooseShot(application::IRandomGenerator& rng) override;
    void applyShotResult(Point point, const ShotResult& result) override;

    AiSnapshot snapshot() const override;
    bool restore(const AiSnapshot& snapshot) override;

private:
    bool isCandidate(Point point) const;
    bool isUnknown(Point point) const;
    void rebuildTargetQueue();
    std::vector<Point> lineExtensions() const;
    void markAroundSunk(const std::vector<Point>& cells);

    std::vector<std::vector<KnowledgeCell>> knowledge_;
    std::deque<Point> targetQueue_;
    std::vector<Point> activeHits_;
};

}
