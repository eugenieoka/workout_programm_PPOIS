#pragma once

#include <string>

namespace workout::app {

/**
 * @brief Command-line and environment configuration.
 *
 * Resolution order:
 *   dbPath:  --db  > WORKOUT_DB  > ~/.workout/workout.db
 *   dataDir: --data-dir > WORKOUT_DATA_DIR > CMake WORKOUT_DATA_DIR > ./data
 */
struct AppOptions {
    std::string dbPath;
    std::string dataDir;
    bool showHelp = false;

    /**
     * @throws std::invalid_argument on unknown or incomplete arguments.
     */
    [[nodiscard]] static AppOptions parse(int argc, char** argv);

    [[nodiscard]] static std::string defaultDbPath();
    [[nodiscard]] static std::string defaultDataDir();
};

} // namespace workout::app
