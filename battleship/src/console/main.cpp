#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "application/CollectionUtils.h"
#include "application/Match.h"
#include "domain/Types.h"
#include "infrastructure/MatchFactory.h"

namespace {

using battleship::application::Match;
using battleship::domain::Board;
using battleship::domain::Cell;
using battleship::domain::Phase;
using battleship::domain::Point;
using battleship::domain::Turn;
using battleship::infrastructure::AiStrategyKind;

struct Statistics {
    int wins = 0;
    int losses = 0;
};

class StatisticsRepository final {
public:
    explicit StatisticsRepository(std::filesystem::path path)
        : path_(std::move(path)) {
    }

    Statistics load() const {
        std::ifstream in(path_);
        Statistics stats;
        if (in) {
            in >> stats.wins >> stats.losses;
        }
        return stats;
    }

    void save(const Statistics& stats) const {
        std::ofstream out(path_);
        if (!out) {
            throw std::runtime_error("Не удалось сохранить статистику.");
        }
        out << stats.wins << ' ' << stats.losses << '\n';
    }

private:
    std::filesystem::path path_;
};

std::optional<Point> parsePoint(const std::string& token) {
    if (token.size() < 2) {
        return std::nullopt;
    }

    const char rowChar = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
    if (rowChar < 'A' || rowChar > 'J') {
        return std::nullopt;
    }

    int col = 0;
    try {
        col = std::stoi(token.substr(1));
    } catch (const std::exception&) {
        return std::nullopt;
    }

    Point point{rowChar - 'A', col - 1};
    if (!Board::isInside(point)) {
        return std::nullopt;
    }
    return point;
}

char cellSymbol(Cell cell, bool ownBoard) {
    switch (cell) {
        case Cell::Empty:
            return '.';
        case Cell::Ship:
            return ownBoard ? '#' : '.';
        case Cell::Miss:
            return 'o';
        case Cell::Hit:
            return 'X';
        case Cell::Sunk:
            return 'Z';
    }
    return '?';
}

void printBoard(const Match& match, bool ownBoard) {
    std::cout << "   1 2 3 4 5 6 7 8 9 10\n";
    for (int row = 0; row < Board::kSize; ++row) {
        std::cout << static_cast<char>('A' + row) << "  ";
        for (int col = 0; col < Board::kSize; ++col) {
            Point point{row, col};
            const Cell cell = ownBoard ? match.playerCellAt(point) : match.enemyRadarCellAt(point);
            std::cout << cellSymbol(cell, ownBoard) << ' ';
        }
        std::cout << '\n';
    }
}

void printState(const Match& match) {
    std::cout << "\n=== " << match.titleText() << " ===\n";
    std::cout << match.statusText() << "\n\n";
    std::cout << "Ваше поле:\n";
    printBoard(match, true);
    std::cout << "\nРадар противника:\n";
    printBoard(match, false);

    const auto& messages = match.messages();
    std::cout << "\nЖурнал: ";
    std::cout << battleship::application::joinMapped(messages, " | ", [](const std::string& text) {
        return text;
    }) << "\n";
}

AiStrategyKind askStrategy() {
    while (true) {
        std::cout << "Выберите ИИ: 1 - случайный, 2 - умный добивающий: ";
        std::string input;
        std::getline(std::cin, input);
        if (input == "1") {
            return AiStrategyKind::Random;
        }
        if (input == "2" || input.empty()) {
            return AiStrategyKind::Hunt;
        }
        std::cout << "Введите 1 или 2.\n";
    }
}

void setupFleet(Match& match) {
    while (match.phase() == Phase::Setup) {
        printState(match);
        std::cout << "\nКоманды подготовки: A1 h, A1 v, auto, clear, start, save\n> ";
        std::string line;
        std::getline(std::cin, line);
        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command == "auto") {
            match.reshufflePlayerFleet();
        } else if (command == "clear") {
            match.clearPlayerFleet();
        } else if (command == "start") {
            match.startBattle();
        } else if (command == "save") {
            match.save();
        } else {
            std::string direction;
            input >> direction;
            std::optional<Point> point = parsePoint(command);
            if (!point.has_value() || (direction != "h" && direction != "v")) {
                std::cout << "Формат: A1 h или A1 v. h - горизонтально, v - вертикально.\n";
                continue;
            }
            match.placePlayerShip(*point, direction == "h");
        }
    }
}

void battleLoop(Match& match) {
    while (!match.isFinished()) {
        printState(match);
        std::cout << "\nКоманды боя: A1, save, quit\n> ";
        std::string command;
        std::getline(std::cin, command);

        if (command == "save") {
            match.save();
            continue;
        }
        if (command == "quit") {
            match.save();
            return;
        }

        std::optional<Point> point = parsePoint(command);
        if (!point.has_value()) {
            std::cout << "Введите координату от A1 до J10.\n";
            continue;
        }

        match.playerShoot(*point);
        while (match.turn() == Turn::Enemy && !match.isFinished()) {
            match.enemyShoot();
        }
    }
    printState(match);
}

std::unique_ptr<Match> makeMatch(AiStrategyKind kind) {
    return std::make_unique<Match>(battleship::infrastructure::createDefaultMatch(kind));
}

}  // namespace

int main() {
    StatisticsRepository statsRepository("battleship_stats.txt");
    Statistics stats = statsRepository.load();
    std::future<void> pendingSave;

    while (true) {
        std::cout << "\nМорской бой\n";
        std::cout << "Победы: " << stats.wins << ", поражения: " << stats.losses << "\n";
        std::cout << "1. Новая игра\n2. Продолжить\n3. Выйти\n> ";

        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "3") {
            break;
        }

        std::unique_ptr<Match> match;
        if (choice == "1") {
            match = makeMatch(askStrategy());
        } else if (choice == "2") {
            match = makeMatch(AiStrategyKind::Hunt);
            if (!match->load()) {
                match = makeMatch(AiStrategyKind::Random);
            }
            if (!match->load()) {
                std::cout << match->lastPersistenceError() << "\n";
                continue;
            }
        } else {
            std::cout << "Введите 1, 2 или 3.\n";
            continue;
        }

        if (match->phase() == Phase::Setup) {
            setupFleet(*match);
        }
        if (match->phase() == Phase::Battle) {
            battleLoop(*match);
        }

        if (match->isFinished()) {
            if (match->playerWon()) {
                ++stats.wins;
            } else {
                ++stats.losses;
            }
            pendingSave = std::async(std::launch::async, [&statsRepository, stats] {
                statsRepository.save(stats);
            });
            pendingSave.get();
        }
    }

    if (pendingSave.valid()) {
        pendingSave.get();
    }
    return 0;
}
