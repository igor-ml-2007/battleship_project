#pragma once

#include <vector>

#include "application/RandomGenerator.h"
#include "domain/Types.h"

namespace battleship::domain {

struct ShipSnapshot {
    std::vector<Point> cells;
    bool sunk = false;
};

struct BoardSnapshot {
    std::vector<std::vector<Cell>> grid;
    std::vector<ShipSnapshot> ships;
};

class Board {
public:
    static constexpr int kSize = 10;

    Board();

    void reset();
    void autoPlaceFleet(application::IRandomGenerator& rng);
    bool tryPlaceShip(Point start, int length, bool horizontal);
    ShotResult fireAt(Point point);
    bool allShipsSunk() const;
    Cell cellAt(Point point) const;

    BoardSnapshot snapshot() const;
    bool restore(const BoardSnapshot& snapshot);

    static bool isInside(Point point);

private:
    struct Ship {
        std::vector<Point> cells;
        bool sunk = false;
    };

    bool canPlaceShip(Point start, int length, bool horizontal) const;
    void placeShip(Point start, int length, bool horizontal);
    int findShipIndex(Point point) const;
    void markSunkShip(Ship& ship);

    std::vector<std::vector<Cell>> grid_;
    std::vector<Ship> ships_;
};

}  // namespace battleship::domain
