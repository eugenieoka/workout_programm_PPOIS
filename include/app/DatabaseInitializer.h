#pragma once

#include <memory>
#include <string>

#include "db/Database.h"

namespace workout::app {

/**
 * @brief Opens the database and applies the existing schema/seed when needed.
 *
 * Both paths are injected by the caller. This class never hardcodes
 * ~/.workout and never changes the schema or seed files.
 */
class DatabaseInitializer {
public:
    DatabaseInitializer(std::string dbPath, std::string dataDir);

    /**
     * @brief Create the parent directory, open the DB, and load schema/seed
     *        when user_version is 0.
     *
     * @throws std::runtime_error if files are missing or the schema version is
     *         unsupported.
     */
    void initialize();

    [[nodiscard]] infra::Database& database();

private:
    [[nodiscard]] static int readUserVersion(infra::Database& db);
    void ensureParentDirectory() const;

    std::string dbPath_;
    std::string dataDir_;
    std::unique_ptr<infra::Database> db_;
};

} // namespace workout::app
