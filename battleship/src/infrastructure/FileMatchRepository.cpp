#include "infrastructure/FileMatchRepository.h"

#include <fstream>
#include <sstream>

#include "application/Match.h"
#include "domain/Board.h"
#include "domain/Types.h"

namespace battleship::infrastructure {
namespace {

using battleship::application::MatchSnapshot;
using battleship::domain::AiSnapshot;
using battleship::domain::Board;
using battleship::domain::BoardSnapshot;
using battleship::domain::Cell;
using battleship::domain::KnowledgeCell;
using battleship::domain::Point;
using battleship::domain::ShipSnapshot;

char cellToChar(Cell cell) {
    switch (cell) {
        case Cell::Empty:
            return 'E';
        case Cell::Ship:
            return 'S';
        case Cell::Miss:
            return 'M';
        case Cell::Hit:
            return 'H';
        case Cell::Sunk:
            return 'K';
    }
    return 'E';
}

std::optional<Cell> charToCell(char value) {
    switch (value) {
        case 'E':
            return Cell::Empty;
        case 'S':
            return Cell::Ship;
        case 'M':
            return Cell::Miss;
        case 'H':
            return Cell::Hit;
        case 'K':
            return Cell::Sunk;
        default:
            return std::nullopt;
    }
}

char knowledgeToChar(KnowledgeCell cell) {
    switch (cell) {
        case KnowledgeCell::Unknown:
            return 'U';
        case KnowledgeCell::Miss:
            return 'M';
        case KnowledgeCell::Hit:
            return 'H';
        case KnowledgeCell::Sunk:
            return 'K';
    }
    return 'U';
}

std::optional<KnowledgeCell> charToKnowledge(char value) {
    switch (value) {
        case 'U':
            return KnowledgeCell::Unknown;
        case 'M':
            return KnowledgeCell::Miss;
        case 'H':
            return KnowledgeCell::Hit;
        case 'K':
            return KnowledgeCell::Sunk;
        default:
            return std::nullopt;
    }
}

void writePoint(std::ostream& out, Point point) {
    out << point.row << ' ' << point.col;
}

bool readPoint(std::istream& in, Point& point) {
    return static_cast<bool>(in >> point.row >> point.col) && Board::isInside(point);
}

void writeOptionalPoint(std::ostream& out, const std::optional<Point>& point) {
    if (!point.has_value()) {
        out << "none\n";
        return;
    }
    out << "some ";
    writePoint(out, *point);
    out << '\n';
}

bool readOptionalPoint(std::istream& in, std::optional<Point>& point) {
    std::string marker;
    if (!(in >> marker)) {
        return false;
    }
    if (marker == "none") {
        point.reset();
        return true;
    }
    if (marker != "some") {
        return false;
    }
    Point value;
    if (!readPoint(in, value)) {
        return false;
    }
    point = value;
    return true;
}

void writeBoard(std::ostream& out, const BoardSnapshot& board) {
    for (const auto& row : board.grid) {
        for (Cell cell : row) {
            out << cellToChar(cell);
        }
        out << '\n';
    }

    out << board.ships.size() << '\n';
    for (const ShipSnapshot& ship : board.ships) {
        out << ship.sunk << ' ' << ship.cells.size();
        for (Point cell : ship.cells) {
            out << ' ';
            writePoint(out, cell);
        }
        out << '\n';
    }
}

bool readBoard(std::istream& in, BoardSnapshot& board) {
    board.grid.assign(Board::kSize, std::vector<Cell>(Board::kSize, Cell::Empty));
    for (int row = 0; row < Board::kSize; ++row) {
        std::string line;
        if (!(in >> line) || line.size() != Board::kSize) {
            return false;
        }
        for (int col = 0; col < Board::kSize; ++col) {
            std::optional<Cell> cell = charToCell(line[col]);
            if (!cell.has_value()) {
                return false;
            }
            board.grid[row][col] = *cell;
        }
    }

    std::size_t shipCount = 0;
    if (!(in >> shipCount)) {
        return false;
    }
    board.ships.clear();
    board.ships.reserve(shipCount);
    for (std::size_t i = 0; i < shipCount; ++i) {
        ShipSnapshot ship;
        std::size_t cellCount = 0;
        if (!(in >> ship.sunk >> cellCount) || cellCount == 0) {
            return false;
        }
        ship.cells.reserve(cellCount);
        for (std::size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
            Point point;
            if (!readPoint(in, point)) {
                return false;
            }
            ship.cells.push_back(point);
        }
        board.ships.push_back(std::move(ship));
    }
    return true;
}

void writeAi(std::ostream& out, const AiSnapshot& ai) {
    for (const auto& row : ai.knowledge) {
        for (KnowledgeCell cell : row) {
            out << knowledgeToChar(cell);
        }
        out << '\n';
    }

    out << ai.targetQueue.size();
    for (Point point : ai.targetQueue) {
        out << ' ';
        writePoint(out, point);
    }
    out << '\n';

    out << ai.activeHits.size();
    for (Point point : ai.activeHits) {
        out << ' ';
        writePoint(out, point);
    }
    out << '\n';
}

bool readAi(std::istream& in, AiSnapshot& ai) {
    ai.knowledge.assign(Board::kSize, std::vector<KnowledgeCell>(Board::kSize, KnowledgeCell::Unknown));
    for (int row = 0; row < Board::kSize; ++row) {
        std::string line;
        if (!(in >> line) || line.size() != Board::kSize) {
            return false;
        }
        for (int col = 0; col < Board::kSize; ++col) {
            std::optional<KnowledgeCell> cell = charToKnowledge(line[col]);
            if (!cell.has_value()) {
                return false;
            }
            ai.knowledge[row][col] = *cell;
        }
    }

    std::size_t queueSize = 0;
    if (!(in >> queueSize)) {
        return false;
    }
    ai.targetQueue.clear();
    for (std::size_t i = 0; i < queueSize; ++i) {
        Point point;
        if (!readPoint(in, point)) {
            return false;
        }
        ai.targetQueue.push_back(point);
    }

    std::size_t hitCount = 0;
    if (!(in >> hitCount)) {
        return false;
    }
    ai.activeHits.clear();
    ai.activeHits.reserve(hitCount);
    for (std::size_t i = 0; i < hitCount; ++i) {
        Point point;
        if (!readPoint(in, point)) {
            return false;
        }
        ai.activeHits.push_back(point);
    }
    return true;
}

}  // namespace

FileMatchRepository::FileMatchRepository(std::filesystem::path savePath)
    : savePath_(std::move(savePath)) {
}

bool FileMatchRepository::save(const MatchSnapshot& snapshot, std::string& error) {
    std::ofstream out(savePath_);
    if (!out) {
        error = "Не удалось открыть файл сохранения.";
        return false;
    }

    out << "BATTLESHIP_SAVE_V1\n";
    out << snapshot.aiStrategyName << '\n';
    out << battleship::domain::toString(snapshot.turn) << '\n';
    out << battleship::domain::toString(snapshot.phase) << '\n';
    out << snapshot.playerWon << ' ' << snapshot.battleStarted << '\n';
    out << snapshot.remainingShips.size();
    for (int length : snapshot.remainingShips) {
        out << ' ' << length;
    }
    out << '\n';
    writeOptionalPoint(out, snapshot.lastPlayerShot);
    writeOptionalPoint(out, snapshot.lastEnemyShot);

    out << snapshot.messages.size() << '\n';
    for (const std::string& message : snapshot.messages) {
        out << message << '\n';
    }

    writeBoard(out, snapshot.playerBoard);
    writeBoard(out, snapshot.enemyBoard);
    writeAi(out, snapshot.ai);
    return static_cast<bool>(out);
}

std::optional<MatchSnapshot> FileMatchRepository::load(std::string& error) {
    std::ifstream in(savePath_);
    if (!in) {
        error = "Сохранение пока не найдено.";
        return std::nullopt;
    }

    MatchSnapshot snapshot;
    std::string header;
    std::string aiStrategyName;
    std::string turn;
    std::string phase;
    if (!(in >> header) || header != "BATTLESHIP_SAVE_V1" ||
        !(in >> aiStrategyName) ||
        !(in >> turn) || !(in >> phase) ||
        !(in >> snapshot.playerWon >> snapshot.battleStarted)) {
        error = "Файл сохранения поврежден.";
        return std::nullopt;
    }
    snapshot.aiStrategyName = aiStrategyName;

    std::size_t remainingCount = 0;
    if (!(in >> remainingCount)) {
        error = "Файл сохранения поврежден.";
        return std::nullopt;
    }
    snapshot.remainingShips.clear();
    snapshot.remainingShips.reserve(remainingCount);
    for (std::size_t i = 0; i < remainingCount; ++i) {
        int length = 0;
        if (!(in >> length) || length < 1 || length > 4) {
            error = "Файл сохранения содержит неправильный флот.";
            return std::nullopt;
        }
        snapshot.remainingShips.push_back(length);
    }

    std::optional<battleship::domain::Turn> parsedTurn = battleship::domain::parseTurn(turn);
    std::optional<battleship::domain::Phase> parsedPhase = battleship::domain::parsePhase(phase);
    if (!parsedTurn.has_value() || !parsedPhase.has_value()) {
        error = "Файл сохранения содержит неизвестный статус игры.";
        return std::nullopt;
    }
    snapshot.turn = *parsedTurn;
    snapshot.phase = *parsedPhase;

    if (!readOptionalPoint(in, snapshot.lastPlayerShot) ||
        !readOptionalPoint(in, snapshot.lastEnemyShot)) {
        error = "Файл сохранения содержит неправильные координаты.";
        return std::nullopt;
    }

    std::size_t messageCount = 0;
    if (!(in >> messageCount)) {
        error = "Файл сохранения поврежден.";
        return std::nullopt;
    }
    std::string line;
    std::getline(in, line);
    snapshot.messages.clear();
    snapshot.messages.reserve(messageCount);
    for (std::size_t i = 0; i < messageCount; ++i) {
        if (!std::getline(in, line)) {
            error = "Файл сохранения поврежден.";
            return std::nullopt;
        }
        snapshot.messages.push_back(line);
    }

    if (!readBoard(in, snapshot.playerBoard) ||
        !readBoard(in, snapshot.enemyBoard) ||
        !readAi(in, snapshot.ai)) {
        error = "Файл сохранения поврежден.";
        return std::nullopt;
    }

    return snapshot;
}

}  // namespace battleship::infrastructure
