#include "application/Match.h"

#include <algorithm>

#include "application/FleetPlan.h"

namespace battleship::application {
namespace {

constexpr std::size_t kMaxMessages = 8;

std::vector<int> makeFleet() {
    std::vector<int> fleet;
    for (int length : standardFleetPlan()) {
        fleet.push_back(length);
    }
    return fleet;
}

}  // namespace

Match::Match(std::shared_ptr<IRandomGenerator> rng,
             std::shared_ptr<IMatchRepository> repository,
             std::unique_ptr<domain::IAiStrategy> aiStrategy)
    : ai_(std::move(aiStrategy)),
      messages_(kMaxMessages),
      rng_(std::move(rng)),
      repository_(std::move(repository)) {
    reset();
}

void Match::reset() {
    playerBoard_.reset();
    enemyBoard_.autoPlaceFleet(*rng_);
    ai_->reset();
    turn_ = domain::Turn::Player;
    phase_ = domain::Phase::Setup;
    playerWon_ = false;
    battleStarted_ = false;
    remainingShips_ = makeFleet();
    lastPlayerShot_.reset();
    lastEnemyShot_.reset();
    lastPersistenceError_.clear();
    messages_.clear();
    pushMessage("Новая партия создана. Расставьте корабли на левом поле.");
    pushMessage("Клик по своему полю ставит текущий корабль. Направление меняется кнопкой \"Повернуть\".");
}

bool Match::reshufflePlayerFleet() {
    if (!canReshuffle()) {
        return false;
    }

    playerBoard_.autoPlaceFleet(*rng_);
    remainingShips_.clear();
    lastEnemyShot_.reset();
    pushMessage("Ваш флот автоматически расставлен. Можно начинать бой.");
    return true;
}

bool Match::clearPlayerFleet() {
    if (phase_ != domain::Phase::Setup || battleStarted_) {
        return false;
    }

    playerBoard_.reset();
    remainingShips_ = makeFleet();
    lastEnemyShot_.reset();
    pushMessage("Поле очищено. Расставьте флот заново.");
    return true;
}

bool Match::placePlayerShip(domain::Point point, bool horizontal) {
    if (!isPlacingFleet()) {
        return false;
    }

    const int length = remainingShips_.front();
    if (!playerBoard_.tryPlaceShip(point, length, horizontal)) {
        pushMessage("Сюда нельзя поставить корабль длиной " + std::to_string(length) + ".");
        return false;
    }

    remainingShips_.erase(remainingShips_.begin());
    if (remainingShips_.empty()) {
        pushMessage("Флот готов. Нажмите \"Начать бой\".");
    } else {
        pushMessage("Корабль длиной " + std::to_string(length) + " поставлен. Следующий: " +
                    std::to_string(remainingShips_.front()) + ".");
    }
    return true;
}

bool Match::startBattle() {
    if (!canStartBattle()) {
        pushMessage("Сначала расставьте все корабли.");
        return false;
    }

    phase_ = domain::Phase::Battle;
    battleStarted_ = true;
    pushMessage("Бой начался. Стреляйте по радару противника справа.");
    return true;
}

domain::ShotResult Match::playerShoot(domain::Point point) {
    domain::ShotResult result;
    if (turn_ != domain::Turn::Player || phase_ != domain::Phase::Battle || isFinished()) {
        return result;
    }

    result = enemyBoard_.fireAt(point);
    if (!result.valid) {
        pushMessage("Выстрел вне поля.");
        return result;
    }

    if (result.alreadyTried) {
        pushMessage("В клетку " + formatPoint(point) + " уже стреляли.");
        return result;
    }

    lastPlayerShot_ = point;

    if (result.hit) {
        if (result.sunk) {
            pushMessage("Корабль противника потоплен в точке " + formatPoint(point) + ".");
        } else {
            pushMessage("Попадание в " + formatPoint(point) + ". Продолжайте атаку.");
        }
    } else {
        pushMessage("Промах в " + formatPoint(point) + ". Ход противника.");
        turn_ = domain::Turn::Enemy;
    }

    if (result.victory) {
        phase_ = domain::Phase::Finished;
        playerWon_ = true;
        pushMessage("Победа! Вражеский флот уничтожен.");
    }

    return result;
}

domain::ShotResult Match::enemyShoot() {
    domain::ShotResult result;
    if (turn_ != domain::Turn::Enemy || isFinished()) {
        return result;
    }

    domain::Point point = ai_->chooseShot(*rng_);
    result = playerBoard_.fireAt(point);
    ai_->applyShotResult(point, result);

    if (!result.valid || result.alreadyTried) {
        return result;
    }

    lastEnemyShot_ = point;

    if (result.hit) {
        if (result.sunk) {
            pushMessage("Противник потопил ваш корабль в точке " + formatPoint(point) + ".");
        } else {
            pushMessage("Противник попал в " + formatPoint(point) + " и добивает цель.");
        }
    } else {
        pushMessage("Противник промахнулся в " + formatPoint(point) + ". Ваш ход.");
        turn_ = domain::Turn::Player;
    }

    if (result.victory) {
        phase_ = domain::Phase::Finished;
        playerWon_ = false;
        pushMessage("Поражение. Ваш флот затонул.");
    }

    return result;
}

bool Match::save() {
    lastPersistenceError_.clear();
    std::string error;
    if (!repository_->save(snapshot(), error)) {
        lastPersistenceError_ = error.empty() ? "Не удалось сохранить игру." : error;
        pushMessage(lastPersistenceError_);
        return false;
    }

    pushMessage("Игра сохранена.");
    return true;
}

bool Match::load() {
    lastPersistenceError_.clear();
    std::string error;
    std::optional<MatchSnapshot> loaded = repository_->load(error);
    if (!loaded.has_value()) {
        lastPersistenceError_ = error.empty() ? "Не удалось загрузить игру." : error;
        pushMessage(lastPersistenceError_);
        return false;
    }

    if (!restore(*loaded)) {
        lastPersistenceError_ = "Файл сохранения поврежден.";
        pushMessage(lastPersistenceError_);
        return false;
    }

    pushMessage("Игра загружена.");
    return true;
}

const std::string& Match::lastPersistenceError() const {
    return lastPersistenceError_;
}

domain::Cell Match::playerCellAt(domain::Point point) const {
    return playerBoard_.cellAt(point);
}

domain::Cell Match::enemyRadarCellAt(domain::Point point) const {
    domain::Cell cell = enemyBoard_.cellAt(point);
    return cell == domain::Cell::Ship ? domain::Cell::Empty : cell;
}

domain::Turn Match::turn() const {
    return turn_;
}

domain::Phase Match::phase() const {
    return phase_;
}

bool Match::isFinished() const {
    return phase_ == domain::Phase::Finished;
}

bool Match::playerWon() const {
    return playerWon_;
}

bool Match::canReshuffle() const {
    return !battleStarted_ && phase_ == domain::Phase::Setup;
}

bool Match::canStartBattle() const {
    return phase_ == domain::Phase::Setup && remainingShips_.empty();
}

bool Match::isPlacingFleet() const {
    return phase_ == domain::Phase::Setup && !remainingShips_.empty();
}

int Match::nextShipLength() const {
    return remainingShips_.empty() ? 0 : remainingShips_.front();
}

const std::vector<int>& Match::remainingShips() const {
    return remainingShips_;
}

const std::vector<std::string>& Match::messages() const {
    return messages_.values();
}

std::string Match::titleText() const {
    if (phase_ == domain::Phase::Finished) {
        return playerWon_ ? "Победа" : "Поражение";
    }
    return "Морской бой";
}

std::string Match::statusText() const {
    if (phase_ == domain::Phase::Finished) {
        return playerWon_ ? "Вы победили. Нажмите \"Новая игра\", чтобы сыграть снова."
                          : "Противник оказался точнее. Нажмите \"Новая игра\" для реванша.";
    }

    if (turn_ == domain::Turn::Enemy) {
        return "Ход противника. Он стреляет автоматически.";
    }

    if (isPlacingFleet()) {
        return "Расставьте корабль длиной " + std::to_string(nextShipLength()) +
               " на левом поле. Можно повернуть направление.";
    }

    if (canStartBattle()) {
        return "Флот готов. Нажмите \"Начать бой\", чтобы открыть радар противника.";
    }

    return "Ваш ход. Кликайте по клеткам на вражеском радаре справа.";
}

std::optional<domain::Point> Match::lastPlayerShot() const {
    return lastPlayerShot_;
}

std::optional<domain::Point> Match::lastEnemyShot() const {
    return lastEnemyShot_;
}

MatchSnapshot Match::snapshot() const {
    return {
        playerBoard_.snapshot(),
        enemyBoard_.snapshot(),
        ai_->snapshot(),
        ai_->name(),
        turn_,
        phase_,
        playerWon_,
        battleStarted_,
        remainingShips_,
        messages_.values(),
        lastPlayerShot_,
        lastEnemyShot_
    };
}

bool Match::restore(const MatchSnapshot& snapshot) {
    domain::Board nextPlayerBoard;
    domain::Board nextEnemyBoard;
    if (snapshot.aiStrategyName != ai_->name()) {
        return false;
    }
    if (!nextPlayerBoard.restore(snapshot.playerBoard) ||
        !nextEnemyBoard.restore(snapshot.enemyBoard) ||
        !ai_->restore(snapshot.ai)) {
        return false;
    }

    playerBoard_ = std::move(nextPlayerBoard);
    enemyBoard_ = std::move(nextEnemyBoard);
    turn_ = snapshot.turn;
    phase_ = snapshot.phase;
    playerWon_ = snapshot.playerWon;
    battleStarted_ = snapshot.battleStarted;
    remainingShips_ = snapshot.remainingShips;
    messages_.assign(snapshot.messages);
    lastPlayerShot_ = snapshot.lastPlayerShot;
    lastEnemyShot_ = snapshot.lastEnemyShot;
    lastPersistenceError_.clear();
    return true;
}

std::string Match::formatPoint(domain::Point point) {
    return std::string(1, static_cast<char>('A' + point.row)) + std::to_string(point.col + 1);
}

void Match::pushMessage(const std::string& text) {
    messages_.push(text);
}

}
