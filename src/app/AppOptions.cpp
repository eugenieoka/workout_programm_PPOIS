#include "app/AppOptions.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "./data"
#endif

namespace workout::app {
namespace {

std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0' ? std::string(value) : fallback;
}

std::string requireValue(int argc, char** argv, int& index, const std::string& option) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("Missing value for " + option);
    }
    ++index;
    std::string value = argv[index];
    if (value.empty()) {
        throw std::invalid_argument("Empty value for " + option);
    }
    return value;
}

} // namespace

AppOptions AppOptions::parse(int argc, char** argv) {
    AppOptions options;
    options.dbPath = envOr("WORKOUT_DB", defaultDbPath());
    options.dataDir = envOr("WORKOUT_DATA_DIR", defaultDataDir());

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
        } else if (arg == "--db") {
            options.dbPath = requireValue(argc, argv, i, arg);
        } else if (arg.rfind("--db=", 0) == 0) {
            options.dbPath = arg.substr(5);
            if (options.dbPath.empty()) {
                throw std::invalid_argument("Empty value for --db");
            }
        } else if (arg == "--data-dir") {
            options.dataDir = requireValue(argc, argv, i, arg);
        } else if (arg.rfind("--data-dir=", 0) == 0) {
            options.dataDir = arg.substr(11);
            if (options.dataDir.empty()) {
                throw std::invalid_argument("Empty value for --data-dir");
            }
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    return options;
}

std::string AppOptions::defaultDbPath() {
    std::string home;
#if defined(_WIN32)
    home = envOr("USERPROFILE", "");
#else
    home = envOr("HOME", "");
#endif

    if (home.empty()) {
        return "workout.db";
    }
    return (std::filesystem::path(home) / ".workout" / "workout.db").string();
}

std::string AppOptions::defaultDataDir() {
    return WORKOUT_DATA_DIR;
}

} // namespace workout::app
