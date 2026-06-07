#pragma once

#include <algorithm>
#include <sstream>
#include <string>

namespace battleship::application {

template <typename Container, typename Predicate>
auto countWhere(const Container& container, Predicate predicate) {
    return std::count_if(container.begin(), container.end(), std::move(predicate));
}

template <typename Container, typename Mapper>
std::string joinMapped(const Container& container, const std::string& separator, Mapper mapper) {
    std::ostringstream out;
    bool first = true;
    for (const auto& item : container) {
        if (!first) {
            out << separator;
        }
        first = false;
        out << mapper(item);
    }
    return out.str();
}

}
