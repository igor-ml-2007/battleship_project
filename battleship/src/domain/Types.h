#pragma once

#include <optional>
#include <string>
#include <vector>

namespace battleship::domain {

struct Point {
    int row = 0;
    int col = 0;

    friend bool operator==(const Point& lhs, const Point& rhs) = default;
};

struct ShotResult {
    bool valid = false;
    bool alreadyTried = false;
    bool hit = false;
    bool sunk = false;
    bool victory = false;
    std::vector<Point> sunkCells;
};

enum class Cell {
    Empty,
    Ship,
    Miss,
    Hit,
    Sunk
};

enum class KnowledgeCell {
    Unknown,
    Miss,
    Hit,
    Sunk
};

enum class Turn {
    Player,
    Enemy
};

enum class Phase {
    Setup,
    Battle,
    Finished
};

std::string toString(Turn turn);
std::string toString(Phase phase);
std::optional<Turn> parseTurn(const std::string& value);
std::optional<Phase> parsePhase(const std::string& value);

}
