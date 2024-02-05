//
// Created by hackerman on 2/4/24.
//

#pragma once
#include "CInotify.hpp"


using FilePositionCache = std::unordered_map<std::string, std::ifstream::pos_type>;

[[nodiscard]] std::function<void(CInotify& self, const CInotify::EventInfo& info)> make_event_handler(
        const std::vector<std::filesystem::path>& watchedFiles);
