#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "application/MatchRepository.h"
#include "application/RandomGenerator.h"
#include "application/BoundedHistory.h"
#include "domain/AiStrategy.h"
#include "domain/Board.h"
#include "domain/Types.h"

namespace battleship::application {

struct MatchSnapshot {
    domain::BoardSnapshot playerBoard;
    domain::BoardSnapshot enemyBoard;
    domain::AiSnapshot ai;
    std::string aiStrategyName = "hunt";
    domain::Turn turn = domain::Turn::Player;
    domain::Phase phase = domain::Phase::Setup;
    bool playerWon = false;
    bool battleStarted = false;
    std::vector<int> remainingShips;
    std::vector<std::string> messages;
    std::optional<domain::Point> lastPlayerShot;
    std::optional<domain::Point> lastEnemyShot;
};

class Match {
public:
    Match(std::shared_ptr<IRandomGenerator> rng,
          std::shared_ptr<IMatchRepository> repository,
          std::unique_ptr<domain::IAiStrategy> aiStrategy);

    void reset();
    bool reshufflePlayerFleet();
    bool clearPlayerFleet();
    bool placePlayerShip(domain::Point point, bool horizontal);
    bool startBattle();
    domain::ShotResult playerShoot(domain::Point point);
    domain::ShotResult enemyShoot();

    bool save();
    bool load();
    const std::string& lastPersistenceError() const;

    domain::Cell playerCellAt(domain::Point point) const;
    domain::Cell enemyRadarCellAt(domain::Point point) const;

    domain::Turn turn() const;
    domain::Phase phase() const;
    bool isFinished() const;
    bool playerWon() const;
    bool canReshuffle() const;
    bool canStartBattle() const;
    bool isPlacingFleet() const;
    int nextShipLength() const;
    const std::vector<int>& remainingShips() const;

    const std::vector<std::string>& messages() const;
    std::string titleText() const;
    std::string statusText() const;

    std::optional<domain::Point> lastPlayerShot() const;
    std::optional<domain::Point> lastEnemyShot() const;
    MatchSnapshot snapshot() const;

private:
    static std::string formatPoint(domain::Point point);

    bool restore(const MatchSnapshot& snapshot);
    void pushMessage(const std::string& text);

    domain::Board playerBoard_;
    domain::Board enemyBoard_;
    std::unique_ptr<domain::IAiStrategy> ai_;
    domain::Turn turn_ = domain::Turn::Player;
    domain::Phase phase_ = domain::Phase::Setup;
    bool playerWon_ = false;
    bool battleStarted_ = false;
    std::vector<int> remainingShips_;
    BoundedHistory<std::string> messages_;
    std::optional<domain::Point> lastPlayerShot_;
    std::optional<domain::Point> lastEnemyShot_;
    std::shared_ptr<IRandomGenerator> rng_;
    std::shared_ptr<IMatchRepository> repository_;
    std::string lastPersistenceError_;
};

}  // namespace battleship::application
