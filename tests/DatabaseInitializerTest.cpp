#include <gtest/gtest.h>

#include "app/DatabaseInitializer.h"
#include "db/Database.h"
#include "db/ExerciseRepository.h"

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "."
#endif

namespace workout::app {
namespace {

class TempDir {
public:
    TempDir() {
        path_ = std::filesystem::temp_directory_path() /
                ("workout_db_init_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()));
        std::filesystem::create_directories(path_);
    }

    ~TempDir() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

TEST(DatabaseInitializerTest, InitializesFileDatabaseAndIsIdempotent) {
    TempDir temp;
    const std::string dbPath = (temp.path() / "workout.db").string();

    {
        DatabaseInitializer initializer(dbPath, WORKOUT_DATA_DIR);
        initializer.initialize();

        infra::Database& db = initializer.database();
        EXPECT_EQ(infra::ExerciseRepository(db).findAll().size(), 27u);

        infra::Statement version(db.getRawConnection(), "PRAGMA user_version;");
        ASSERT_TRUE(version.step());
        EXPECT_EQ(version.columnInt(0), 1);
    }

    {
        DatabaseInitializer initializer(dbPath, WORKOUT_DATA_DIR);
        initializer.initialize();
        EXPECT_EQ(infra::ExerciseRepository(initializer.database()).findAll().size(), 27u);
    }
}

TEST(DatabaseInitializerTest, InitializesInMemoryDatabase) {
    DatabaseInitializer initializer(":memory:", WORKOUT_DATA_DIR);
    initializer.initialize();
    EXPECT_EQ(infra::ExerciseRepository(initializer.database()).findAll().size(), 27u);
}

TEST(DatabaseInitializerTest, RejectsUnsupportedSchemaVersion) {
    TempDir temp;
    const std::string dbPath = (temp.path() / "workout.db").string();

    {
        infra::Database db(dbPath);
        db.execute("PRAGMA user_version = 99;");
    }

    DatabaseInitializer initializer(dbPath, WORKOUT_DATA_DIR);
    EXPECT_THROW(initializer.initialize(), std::runtime_error);
}

} // namespace
} // namespace workout::app
