#include "domain/AiPlayer.h"

#include <algorithm>
#include <array>

#include "domain/Board.h"

namespace battleship::domain {
namespace {

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

bool isValidKnowledgeGrid(const std::vector<std::vector<KnowledgeCell>>& knowledge) {
    if (knowledge.size() != Board::kSize) {
        return false;
    }
    return std::all_of(knowledge.begin(), knowledge.end(), [](const auto& row) {
        return row.size() == Board::kSize;
    });
}

}  // namespace

AIPlayer::AIPlayer() {
    reset();
}

std::string AIPlayer::name() const {
    return "hunt";
}

void AIPlayer::reset() {
    knowledge_.assign(Board::kSize, std::vector<KnowledgeCell>(Board::kSize, KnowledgeCell::Unknown));
    targetQueue_.clear();
    activeHits_.clear();
}

bool AIPlayer::isUnknown(Point point) const {
    return Board::isInside(point) && knowledge_[point.row][point.col] == KnowledgeCell::Unknown;
}

bool AIPlayer::isCandidate(Point point) const {
    return isUnknown(point);
}

std::vector<Point> AIPlayer::lineExtensions() const {
    std::vector<Point> result;
    if (activeHits_.size() < 2) {
        return result;
    }

    bool sameRow = true;
    bool sameCol = true;
    for (std::size_t i = 1; i < activeHits_.size(); ++i) {
        sameRow = sameRow && activeHits_[i].row == activeHits_[0].row;
        sameCol = sameCol && activeHits_[i].col == activeHits_[0].col;
    }

    if (sameRow) {
        auto [minIt, maxIt] = std::minmax_element(activeHits_.begin(), activeHits_.end(),
            [](Point lhs, Point rhs) { return lhs.col < rhs.col; });
        result.push_back({minIt->row, minIt->col - 1});
        result.push_back({maxIt->row, maxIt->col + 1});
    } else if (sameCol) {
        auto [minIt, maxIt] = std::minmax_element(activeHits_.begin(), activeHits_.end(),
            [](Point lhs, Point rhs) { return lhs.row < rhs.row; });
        result.push_back({minIt->row - 1, minIt->col});
        result.push_back({maxIt->row + 1, maxIt->col});
    }

    return result;
}

void AIPlayer::rebuildTargetQueue() {
    std::deque<Point> rebuilt;
    for (Point candidate : lineExtensions()) {
        if (isCandidate(candidate)) {
            rebuilt.push_back(candidate);
        }
    }

    if (rebuilt.empty()) {
        for (Point hit : activeHits_) {
            const std::array<Point, 4> neighbors{{
                {hit.row - 1, hit.col},
                {hit.row + 1, hit.col},
                {hit.row, hit.col - 1},
                {hit.row, hit.col + 1},
            }};

            for (Point neighbor : neighbors) {
                if (isCandidate(neighbor) &&
                    std::find(rebuilt.begin(), rebuilt.end(), neighbor) == rebuilt.end()) {
                    rebuilt.push_back(neighbor);
                }
            }
        }
    }

    targetQueue_ = rebuilt;
}

void AIPlayer::markAroundSunk(const std::vector<Point>& cells) {
    for (Point cell : cells) {
        for (Point neighbor : surroundingPoints(cell)) {
            if (Board::isInside(neighbor) && knowledge_[neighbor.row][neighbor.col] == KnowledgeCell::Unknown) {
                knowledge_[neighbor.row][neighbor.col] = KnowledgeCell::Miss;
            }
        }
    }
}

Point AIPlayer::chooseShot(application::IRandomGenerator& rng) {
    while (!targetQueue_.empty()) {
        Point point = targetQueue_.front();
        targetQueue_.pop_front();
        if (isUnknown(point)) {
            return point;
        }
    }

    std::vector<Point> checkerboard;
    std::vector<Point> fallback;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            Point point{row, col};
            if (!isUnknown(point)) {
                continue;
            }
            fallback.push_back(point);
            if ((row + col) % 2 == 0) {
                checkerboard.push_back(point);
            }
        }
    }

    const auto& pool = checkerboard.empty() ? fallback : checkerboard;
    return pool[rng.nextInt(0, static_cast<int>(pool.size()) - 1)];
}

void AIPlayer::applyShotResult(Point point, const ShotResult& result) {
    if (!result.valid || result.alreadyTried) {
        return;
    }

    knowledge_[point.row][point.col] = result.hit ? KnowledgeCell::Hit : KnowledgeCell::Miss;

    if (result.hit && !result.sunk) {
        if (std::find(activeHits_.begin(), activeHits_.end(), point) == activeHits_.end()) {
            activeHits_.push_back(point);
        }
        rebuildTargetQueue();
        return;
    }

    if (result.sunk) {
        for (Point cell : result.sunkCells) {
            knowledge_[cell.row][cell.col] = KnowledgeCell::Sunk;
        }
        markAroundSunk(result.sunkCells);
        activeHits_.clear();
        targetQueue_.clear();
        return;
    }

    rebuildTargetQueue();
}

AiSnapshot AIPlayer::snapshot() const {
    return {knowledge_, targetQueue_, activeHits_};
}

bool AIPlayer::restore(const AiSnapshot& snapshot) {
    if (!isValidKnowledgeGrid(snapshot.knowledge)) {
        return false;
    }
    for (Point point : snapshot.targetQueue) {
        if (!Board::isInside(point)) {
            return false;
        }
    }
    for (Point point : snapshot.activeHits) {
        if (!Board::isInside(point)) {
            return false;
        }
    }

    knowledge_ = snapshot.knowledge;
    targetQueue_ = snapshot.targetQueue;
    activeHits_ = snapshot.activeHits;
    return true;
}

}  // namespace battleship::domain
