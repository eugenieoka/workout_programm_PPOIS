#include <gtest/gtest.h>

#include "app/AppOptions.h"

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace workout::app {
namespace {

AppOptions parse(std::initializer_list<std::string> arguments) {
    std::vector<std::string> storage;
    storage.emplace_back("workout_cli");
    for (const std::string& argument : arguments) {
        storage.push_back(argument);
    }

    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (std::string& argument : storage) {
        argv.push_back(argument.data());
    }

    return AppOptions::parse(static_cast<int>(argv.size()), argv.data());
}

TEST(AppOptionsTest, ParsesExplicitPaths) {
    const AppOptions options = parse({"--db", "/tmp/workout.db",
                                      "--data-dir", "/tmp/workout-data"});
    EXPECT_EQ(options.dbPath, "/tmp/workout.db");
    EXPECT_EQ(options.dataDir, "/tmp/workout-data");
    EXPECT_FALSE(options.showHelp);
}

TEST(AppOptionsTest, ParsesEqualsSyntax) {
    const AppOptions options = parse({"--db=/tmp/workout.db",
                                      "--data-dir=/tmp/workout-data"});
    EXPECT_EQ(options.dbPath, "/tmp/workout.db");
    EXPECT_EQ(options.dataDir, "/tmp/workout-data");
}

TEST(AppOptionsTest, ParsesHelp) {
    const AppOptions options = parse({"--help"});
    EXPECT_TRUE(options.showHelp);
}

TEST(AppOptionsTest, RejectsUnknownArgument) {
    EXPECT_THROW(parse({"--unknown"}), std::invalid_argument);
}

TEST(AppOptionsTest, RejectsMissingValue) {
    EXPECT_THROW(parse({"--db"}), std::invalid_argument);
    EXPECT_THROW(parse({"--data-dir"}), std::invalid_argument);
    EXPECT_THROW(parse({"--db="}), std::invalid_argument);
    EXPECT_THROW(parse({"--data-dir="}), std::invalid_argument);
}

TEST(AppOptionsTest, DefaultsAreNonEmpty) {
    EXPECT_FALSE(AppOptions::defaultDbPath().empty());
    EXPECT_FALSE(AppOptions::defaultDataDir().empty());
}

} // namespace
} // namespace workout::app
