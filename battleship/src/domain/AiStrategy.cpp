#include "domain/AiStrategy.h"

#include <algorithm>

#include "domain/Board.h"

namespace battleship::domain {
namespace {

bool isValidKnowledgeGrid(const std::vector<std::vector<KnowledgeCell>>& knowledge) {
    if (knowledge.size() != Board::kSize) {
        return false;
    }
    return std::all_of(knowledge.begin(), knowledge.end(), [](const auto& row) {
        return row.size() == Board::kSize;
    });
}

}

RandomAiStrategy::RandomAiStrategy() {
    reset();
}

std::string RandomAiStrategy::name() const {
    return "random";
}

void RandomAiStrategy::reset() {
    knowledge_.assign(Board::kSize, std::vector<KnowledgeCell>(Board::kSize, KnowledgeCell::Unknown));
}

Point RandomAiStrategy::chooseShot(application::IRandomGenerator& rng) {
    std::vector<Point> candidates;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            if (knowledge_[row][col] == KnowledgeCell::Unknown) {
                candidates.push_back({row, col});
            }
        }
    }
    return candidates[rng.nextInt(0, static_cast<int>(candidates.size()) - 1)];
}

void RandomAiStrategy::applyShotResult(Point point, const ShotResult& result) {
    if (!result.valid || result.alreadyTried) {
        return;
    }
    knowledge_[point.row][point.col] = result.hit ? KnowledgeCell::Hit : KnowledgeCell::Miss;
    if (result.sunk) {
        for (Point cell : result.sunkCells) {
            knowledge_[cell.row][cell.col] = KnowledgeCell::Sunk;
        }
    }
}

AiSnapshot RandomAiStrategy::snapshot() const {
    return {knowledge_, {}, {}};
}

bool RandomAiStrategy::restore(const AiSnapshot& snapshot) {
    if (!isValidKnowledgeGrid(snapshot.knowledge)) {
        return false;
    }
    knowledge_ = snapshot.knowledge;
    return true;
}

}
