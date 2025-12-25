#pragma once

#include <vector>
#include <string>

struct TeamStats {
    int maxMP = 10;
    int currentMP = 10;

    int maxAP = 10;
    int currentAP = 10;
};

struct TeamConfig {
    std::vector<std::string> startingUnitFiles;

    void AddFile(const std::string& path) {
        startingUnitFiles.push_back(path);
    }
};