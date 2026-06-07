#include "application/FleetPlan.h"

namespace battleship::application {

Generator<int> standardFleetPlan() {
    co_yield 4;
    co_yield 3;
    co_yield 3;
    co_yield 2;
    co_yield 2;
    co_yield 2;
    co_yield 1;
    co_yield 1;
    co_yield 1;
    co_yield 1;
}

}  // namespace battleship::application
