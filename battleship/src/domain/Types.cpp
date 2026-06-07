#include "domain/Types.h"

namespace battleship::domain {

std::string toString(Turn turn) {
    return turn == Turn::Player ? "Player" : "Enemy";
}

std::string toString(Phase phase) {
    switch (phase) {
        case Phase::Setup:
            return "Setup";
        case Phase::Battle:
            return "Battle";
        case Phase::Finished:
            return "Finished";
    }
    return "Setup";
}

std::optional<Turn> parseTurn(const std::string& value) {
    if (value == "Player") {
        return Turn::Player;
    }
    if (value == "Enemy") {
        return Turn::Enemy;
    }
    return std::nullopt;
}

std::optional<Phase> parsePhase(const std::string& value) {
    if (value == "Setup") {
        return Phase::Setup;
    }
    if (value == "Battle") {
        return Phase::Battle;
    }
    if (value == "Finished") {
        return Phase::Finished;
    }
    return std::nullopt;
}

}
