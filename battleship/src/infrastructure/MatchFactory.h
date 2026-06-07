#pragma once

#include "application/Match.h"

namespace battleship::infrastructure {

enum class AiStrategyKind {
    Random,
    Hunt
};

application::Match createDefaultMatch(AiStrategyKind kind = AiStrategyKind::Hunt);

}  // namespace battleship::infrastructure
