#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "application/MatchRepository.h"

namespace battleship::infrastructure {

class FileMatchRepository final : public application::IMatchRepository {
public:
    explicit FileMatchRepository(std::filesystem::path savePath);

    bool save(const application::MatchSnapshot& snapshot, std::string& error) override;
    std::optional<application::MatchSnapshot> load(std::string& error) override;

private:
    std::filesystem::path savePath_;
};

}  // namespace battleship::infrastructure
