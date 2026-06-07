#include "infrastructure/MatchFactory.h"

#include <filesystem>
#include <memory>

#include "domain/AiPlayer.h"
#include "domain/AiStrategy.h"
#include "infrastructure/FileMatchRepository.h"
#include "infrastructure/StdRandomGenerator.h"

namespace battleship::infrastructure {

application::Match createDefaultMatch(AiStrategyKind kind) {
    auto rng = std::make_shared<StdRandomGenerator>();
    auto repository = std::make_shared<FileMatchRepository>(std::filesystem::path("battleship_save.txt"));
    std::unique_ptr<domain::IAiStrategy> strategy;
    if (kind == AiStrategyKind::Random) {
        strategy = std::make_unique<domain::RandomAiStrategy>();
    } else {
        strategy = std::make_unique<domain::AIPlayer>();
    }
    return application::Match(std::move(rng), std::move(repository), std::move(strategy));
}

}  // namespace battleship::infrastructure
