#include "app/DatabaseInitializer.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace workout::app {
namespace {

constexpr int kSupportedSchemaVersion = 1;

bool isInMemoryPath(const std::string& path) {
    return path == ":memory:" || path.rfind("file:", 0) == 0;
}

bool tableExists(infra::Database& db, const std::string& name) {
    infra::Statement stmt(db.getRawConnection(),
                          "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?;");
    stmt.bind(1, name);
    return stmt.step();
}

} // namespace

DatabaseInitializer::DatabaseInitializer(std::string dbPath, std::string dataDir)
    : dbPath_(std::move(dbPath)), dataDir_(std::move(dataDir)) {}

void DatabaseInitializer::initialize() {
    if (db_) {
        return;
    }

    ensureParentDirectory();
    db_ = std::make_unique<infra::Database>(dbPath_);

    int version = readUserVersion(*db_);
    if (version < 0 || version > kSupportedSchemaVersion) {
        throw std::runtime_error(
            "Unsupported database schema version " + std::to_string(version) +
            " (expected " + std::to_string(kSupportedSchemaVersion) +
            "). Recreate the database or add a migration.");
    }

    if (version == 0 || !tableExists(*db_, "exercises") ||
        !tableExists(*db_, "user_profiles") ||
        !tableExists(*db_, "workout_plans") ||
        !tableExists(*db_, "plan_days") ||
        !tableExists(*db_, "plan_exercises")) {
        db_->executeFile((std::filesystem::path(dataDir_) / "schema.sql").string());
        db_->executeFile((std::filesystem::path(dataDir_) / "seed.sql").string());
        version = readUserVersion(*db_);
    }

    if (version != kSupportedSchemaVersion) {
        throw std::runtime_error(
            "Unsupported database schema version " + std::to_string(version) +
            " (expected " + std::to_string(kSupportedSchemaVersion) +
            "). Recreate the database or add a migration.");
    }
}

infra::Database& DatabaseInitializer::database() {
    if (!db_) {
        throw std::logic_error("DatabaseInitializer::database: initialize() was not called");
    }
    return *db_;
}

int DatabaseInitializer::readUserVersion(infra::Database& db) {
    infra::Statement stmt(db.getRawConnection(), "PRAGMA user_version;");
    if (!stmt.step()) {
        throw std::runtime_error("Failed to read PRAGMA user_version");
    }
    return stmt.columnInt(0);
}

void DatabaseInitializer::ensureParentDirectory() const {
    if (isInMemoryPath(dbPath_)) {
        return;
    }

    const std::filesystem::path parent = std::filesystem::path(dbPath_).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

} // namespace workout::app
