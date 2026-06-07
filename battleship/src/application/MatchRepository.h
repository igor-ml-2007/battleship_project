#pragma once

#include <optional>
#include <string>

namespace battleship::application {

struct MatchSnapshot;

class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    virtual bool save(const MatchSnapshot& snapshot, std::string& error) = 0;
    virtual std::optional<MatchSnapshot> load(std::string& error) = 0;
};

}
