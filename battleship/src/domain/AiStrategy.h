#pragma once

#include <deque>
#include <string>
#include <vector>

#include "application/RandomGenerator.h"
#include "domain/Types.h"

namespace battleship::domain {

struct AiSnapshot {
    std::vector<std::vector<KnowledgeCell>> knowledge;
    std::deque<Point> targetQueue;
    std::vector<Point> activeHits;
};

class IAiStrategy {
public:
    virtual ~IAiStrategy() = default;

    virtual std::string name() const = 0;
    virtual void reset() = 0;
    virtual Point chooseShot(application::IRandomGenerator& rng) = 0;
    virtual void applyShotResult(Point point, const ShotResult& result) = 0;
    virtual AiSnapshot snapshot() const = 0;
    virtual bool restore(const AiSnapshot& snapshot) = 0;
};

class RandomAiStrategy final : public IAiStrategy {
public:
    RandomAiStrategy();

    std::string name() const override;
    void reset() override;
    Point chooseShot(application::IRandomGenerator& rng) override;
    void applyShotResult(Point point, const ShotResult& result) override;
    AiSnapshot snapshot() const override;
    bool restore(const AiSnapshot& snapshot) override;

private:
    std::vector<std::vector<KnowledgeCell>> knowledge_;
};

}
