#include "domain/Board.h"

#include <algorithm>
#include <array>

namespace battleship::domain {
namespace {

constexpr std::array<int, 10> kFleet{4, 3, 3, 2, 2, 2, 1, 1, 1, 1};

std::vector<Point> surroundingPoints(Point point) {
    std::vector<Point> result;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            result.push_back({point.row + dr, point.col + dc});
        }
    }
    return result;
}

}

Board::Board() {
    reset();
}

void Board::reset() {
    grid_.assign(kSize, std::vector<Cell>(kSize, Cell::Empty));
    ships_.clear();
}

bool Board::isInside(Point point) {
    return point.row >= 0 && point.row < kSize && point.col >= 0 && point.col < kSize;
}

Cell Board::cellAt(Point point) const {
    return grid_[point.row][point.col];
}

bool Board::canPlaceShip(Point start, int length, bool horizontal) const {
    for (int i = 0; i < length; ++i) {
        Point current{start.row + (horizontal ? 0 : i), start.col + (horizontal ? i : 0)};
        if (!isInside(current)) {
            return false;
        }

        for (Point neighbor : surroundingPoints(current)) {
            if (isInside(neighbor) && grid_[neighbor.row][neighbor.col] == Cell::Ship) {
                return false;
            }
        }

        if (grid_[current.row][current.col] != Cell::Empty) {
            return false;
        }
    }

    return true;
}

void Board::placeShip(Point start, int length, bool horizontal) {
    Ship ship;
    ship.cells.reserve(length);

    for (int i = 0; i < length; ++i) {
        Point current{start.row + (horizontal ? 0 : i), start.col + (horizontal ? i : 0)};
        grid_[current.row][current.col] = Cell::Ship;
        ship.cells.push_back(current);
    }

    ships_.push_back(ship);
}

void Board::autoPlaceFleet(application::IRandomGenerator& rng) {
    reset();

    for (int length : kFleet) {
        while (true) {
            const bool horizontal = rng.nextInt(0, 1) == 0;
            Point start{rng.nextInt(0, kSize - 1), rng.nextInt(0, kSize - 1)};
            if (!canPlaceShip(start, length, horizontal)) {
                continue;
            }
            placeShip(start, length, horizontal);
            break;
        }
    }
}

bool Board::tryPlaceShip(Point start, int length, bool horizontal) {
    if (!canPlaceShip(start, length, horizontal)) {
        return false;
    }

    placeShip(start, length, horizontal);
    return true;
}

int Board::findShipIndex(Point point) const {
    for (int index = 0; index < static_cast<int>(ships_.size()); ++index) {
        const auto& cells = ships_[index].cells;
        if (std::find(cells.begin(), cells.end(), point) != cells.end()) {
            return index;
        }
    }

    return -1;
}

void Board::markSunkShip(Ship& ship) {
    ship.sunk = true;
    for (Point cell : ship.cells) {
        grid_[cell.row][cell.col] = Cell::Sunk;
    }

    for (Point cell : ship.cells) {
        for (Point neighbor : surroundingPoints(cell)) {
            if (isInside(neighbor) && grid_[neighbor.row][neighbor.col] == Cell::Empty) {
                grid_[neighbor.row][neighbor.col] = Cell::Miss;
            }
        }
    }
}

ShotResult Board::fireAt(Point point) {
    ShotResult result;
    if (!isInside(point)) {
        return result;
    }

    result.valid = true;
    Cell& cell = grid_[point.row][point.col];
    if (cell == Cell::Miss || cell == Cell::Hit || cell == Cell::Sunk) {
        result.alreadyTried = true;
        return result;
    }

    if (cell == Cell::Empty) {
        cell = Cell::Miss;
        return result;
    }

    cell = Cell::Hit;
    result.hit = true;

    const int shipIndex = findShipIndex(point);
    if (shipIndex < 0) {
        return result;
    }

    Ship& ship = ships_[shipIndex];
    const bool sunk = std::all_of(ship.cells.begin(), ship.cells.end(), [&](Point shipCell) {
        Cell state = grid_[shipCell.row][shipCell.col];
        return state == Cell::Hit || state == Cell::Sunk;
    });

    if (!sunk) {
        return result;
    }

    result.sunk = true;
    result.sunkCells = ship.cells;
    markSunkShip(ship);
    result.victory = allShipsSunk();
    return result;
}

bool Board::allShipsSunk() const {
    return !ships_.empty() && std::all_of(ships_.begin(), ships_.end(), [](const Ship& ship) {
        return ship.sunk;
    });
}

BoardSnapshot Board::snapshot() const {
    BoardSnapshot result;
    result.grid = grid_;
    result.ships.reserve(ships_.size());
    for (const Ship& ship : ships_) {
        result.ships.push_back({ship.cells, ship.sunk});
    }
    return result;
}

bool Board::restore(const BoardSnapshot& snapshot) {
    if (snapshot.grid.size() != kSize) {
        return false;
    }
    for (const auto& row : snapshot.grid) {
        if (row.size() != kSize) {
            return false;
        }
    }

    std::vector<Ship> restoredShips;
    restoredShips.reserve(snapshot.ships.size());
    for (const ShipSnapshot& shipSnapshot : snapshot.ships) {
        if (shipSnapshot.cells.empty()) {
            return false;
        }
        for (Point cell : shipSnapshot.cells) {
            if (!isInside(cell)) {
                return false;
            }
        }
        restoredShips.push_back({shipSnapshot.cells, shipSnapshot.sunk});
    }

    grid_ = snapshot.grid;
    ships_ = std::move(restoredShips);
    return true;
}

}
