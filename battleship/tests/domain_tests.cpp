#include <cassert>
#include <memory>
#include <random>
#include <string>

#include "application/Match.h"
#include "application/MatchRepository.h"
#include "application/RandomGenerator.h"
#include "application/CollectionUtils.h"
#include "domain/AiPlayer.h"
#include "domain/AiStrategy.h"
#include "domain/Board.h"
#include "domain/Types.h"

namespace {

class CyclingRandomGenerator final : public battleship::application::IRandomGenerator {
public:
    CyclingRandomGenerator()
        : rng_(42) {
    }

    int nextInt(int minInclusive, int maxInclusive) override {
        std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
        return distribution(rng_);
    }

private:
    std::mt19937 rng_;
};

class MemoryRepository final : public battleship::application::IMatchRepository {
public:
    bool save(const battleship::application::MatchSnapshot& snapshot, std::string& error) override {
        (void)error;
        snapshot_ = snapshot;
        return true;
    }

    std::optional<battleship::application::MatchSnapshot> load(std::string& error) override {
        (void)error;
        return snapshot_;
    }

private:
    std::optional<battleship::application::MatchSnapshot> snapshot_;
};

int countShips(const battleship::domain::Board& board) {
    int result = 0;
    for (int row = 0; row < battleship::domain::Board::kSize; ++row) {
        for (int col = 0; col < battleship::domain::Board::kSize; ++col) {
            if (board.cellAt({row, col}) == battleship::domain::Cell::Ship) {
                ++result;
            }
        }
    }
    return result;
}

}  // namespace

int main() {
    auto rng = std::make_shared<CyclingRandomGenerator>();
    auto repository = std::make_shared<MemoryRepository>();

    battleship::domain::Board board;
    board.autoPlaceFleet(*rng);
    assert(countShips(board) == 20);
    assert(battleship::application::countWhere(board.snapshot().ships, [](const auto& ship) {
        return ship.cells.size() == 1;
    }) == 4);

    battleship::domain::ShotResult outside = board.fireAt({-1, 0});
    assert(!outside.valid);

    battleship::application::Match match(rng, repository, std::make_unique<battleship::domain::AIPlayer>());
    assert(match.turn() == battleship::domain::Turn::Player);
    assert(match.canReshuffle());
    assert(match.isPlacingFleet());
    assert(match.save());

    assert(match.reshufflePlayerFleet());
    assert(match.canStartBattle());
    assert(match.startBattle());
    assert(!match.canReshuffle());
    assert(match.load());
    assert(match.canReshuffle());
    assert(match.isPlacingFleet());

    battleship::domain::RandomAiStrategy randomAi;
    battleship::domain::Point randomShot = randomAi.chooseShot(*rng);
    assert(battleship::domain::Board::isInside(randomShot));

    return 0;
}
