#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "app/WorkoutService.h"
#include "core/User.h"
#include "db/Database.h"
#include "ui/ConsoleUI.h"

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "."
#endif

namespace workout::ui {
namespace {

constexpr uint16_t kAllEquipmentBits = 0x07FF;

class ConsoleUITest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<infra::Database>(":memory:");
        db_->executeFile(std::string(WORKOUT_DATA_DIR) + "/schema.sql");
        db_->executeFile(std::string(WORKOUT_DATA_DIR) + "/seed.sql");
        service_ = std::make_unique<app::WorkoutService>(*db_);
    }

    int64_t saveProfile(const std::string& name = "Ivan") {
        core::UserConstraints constraints;
        constraints.availableEquipment =
            static_cast<core::Equipment>(kAllEquipmentBits);

        const auto profile = core::UserProfile::create(
            name, 30, core::Gender::Male, 180.0, 80.0,
            core::ExperienceLevel::Intermediate, core::TrainingGoal::Strength,
            core::TrainingFrequency::Days3, constraints, -1.0, 24);
        if (!profile) {
            throw std::runtime_error("test profile is invalid");
        }
        return service_->saveProfile(*profile);
    }

    std::string run(const std::string& input) {
        std::istringstream in(input);
        std::ostringstream out;
        ConsoleUI console(*service_, in, out);
        console.run();
        return out.str();
    }

    std::unique_ptr<infra::Database> db_;
    std::unique_ptr<app::WorkoutService> service_;
};

TEST_F(ConsoleUITest, MainMenuExit) {
    const std::string output = run("0\n");
    EXPECT_NE(output.find("Главное меню"), std::string::npos);
    EXPECT_NE(output.find("До свидания"), std::string::npos);
}

TEST_F(ConsoleUITest, InputClosedExitsGracefully) {
    const std::string output = run("");
    EXPECT_NE(output.find("Ввод завершён"), std::string::npos);
}

TEST_F(ConsoleUITest, InvalidMenuChoiceRetries) {
    const std::string output = run("99\n0\n");
    EXPECT_NE(output.find("вне допустимого диапазона"), std::string::npos);
    EXPECT_NE(output.find("До свидания"), std::string::npos);
}

TEST_F(ConsoleUITest, ListsProfiles) {
    (void)saveProfile("Ivan");
    const std::string output = run("1\n1\n0\n0\n");
    EXPECT_NE(output.find("Ivan"), std::string::npos);
    EXPECT_NE(output.find("Профили"), std::string::npos);
}

TEST_F(ConsoleUITest, CreatesProfileThroughMenu) {
    std::string input = "1\n2\nUI User\n";
    input += std::string(12, '\n');
    input += "0\n0\n";

    const std::string output = run(input);
    EXPECT_NE(output.find("Профиль создан"), std::string::npos);

    const auto profiles = service_->listProfiles();
    ASSERT_EQ(profiles.size(), 1u);
    EXPECT_EQ(profiles.front().getName(), "UI User");
}

TEST_F(ConsoleUITest, EditsProfileWeight) {
    const int64_t userId = saveProfile();
    const std::string output = run("1\n3\n1\n4\n5\n85\n0\n0\n0\n");
    EXPECT_NE(output.find("Профиль сохранён"), std::string::npos);

    const auto profile = service_->getProfile(userId);
    ASSERT_TRUE(profile.has_value());
    EXPECT_DOUBLE_EQ(profile->getBodyMetrics().weightKg, 85.0);
}

TEST_F(ConsoleUITest, SetsOneRepMax) {
    const int64_t userId = saveProfile();
    const std::string output = run("1\n3\n1\n5\n1\n1\n100\n0\n0\n0\n");
    EXPECT_NE(output.find("1ПМ сохранён"), std::string::npos);

    const auto maxes = service_->getOneRepMaxes(userId);
    ASSERT_EQ(maxes.size(), 1u);
    EXPECT_DOUBLE_EQ(maxes.at(1), 100.0);
}

TEST_F(ConsoleUITest, CatalogListsExercises) {
    const std::string output = run("2\n1\n0\n0\n");
    EXPECT_NE(output.find("сложность"), std::string::npos);
    EXPECT_EQ(output.find("Ничего не найдено"), std::string::npos) << output;
}

TEST_F(ConsoleUITest, GeneratesAndSavesPlan) {
    const int64_t userId = saveProfile();
    const std::string output = run("1\n3\n1\n0\n3\nд\nUI Plan\nд\n0\n");
    EXPECT_NE(output.find("План сохранён"), std::string::npos) << output;

    const auto plans = service_->listPlans(userId);
    ASSERT_EQ(plans.size(), 1u);
    EXPECT_EQ(plans.front().name, "UI Plan");
    EXPECT_TRUE(plans.front().isActive);
    EXPECT_EQ(plans.front().daysPerWeek, 3);
}


TEST_F(ConsoleUITest, EditsGoalAndHealthConstraints) {
    const int64_t userId = saveProfile();
    const std::string output =
        run("1\n3\n1\n4\n9\n3\n12\n2\n0\n0\n0\n");
    EXPECT_NE(output.find("Профиль сохранён"), std::string::npos) << output;

    const auto profile = service_->getProfile(userId);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->getGoal(), core::TrainingGoal::WeightLoss);
    EXPECT_TRUE(profile->getConstraints().hasConstraint(
        core::HealthConstraint::KneePain));
}

TEST_F(ConsoleUITest, RemovesOneRepMax) {
    const int64_t userId = saveProfile();
    service_->setOneRepMax(userId, 1, 100.0);

    const std::string output = run("1\n3\n1\n5\n2\n1\n0\n0\n0\n");
    EXPECT_NE(output.find("1ПМ удалён"), std::string::npos) << output;
    EXPECT_TRUE(service_->getOneRepMaxes(userId).empty());
}

TEST_F(ConsoleUITest, CatalogSearchAndExerciseCard) {
    const std::string output = run("2\n2\nsquat\n3\n1\n0\n0\n");
    EXPECT_NE(output.find("Barbell Back Squat"), std::string::npos) << output;
    EXPECT_NE(output.find("Сложность: 4/5"), std::string::npos) << output;
}

TEST_F(ConsoleUITest, ViewsActivatesAndDeletesPlan) {
    const int64_t userId = saveProfile();
    const auto profile = service_->getProfile(userId);
    ASSERT_TRUE(profile.has_value());
    const core::WorkoutPlan generated = service_->generatePlan(*profile);
    ASSERT_FALSE(generated.isEmpty());
    const int64_t planId =
        service_->saveGeneratedPlan(*profile, generated, "Saved Plan", false);

    const std::string output =
        run("1\n3\n1\n0\n4\n2\n1\n3\n1\n4\n1\nд\n0\n0\n");
    EXPECT_NE(output.find("План активирован"), std::string::npos) << output;
    EXPECT_NE(output.find("План удалён"), std::string::npos) << output;
    EXPECT_FALSE(service_->getPlan(planId).has_value());
}

TEST_F(ConsoleUITest, ShowsSavedPlans) {
    const int64_t userId = saveProfile();
    const auto profile = service_->getProfile(userId);
    ASSERT_TRUE(profile.has_value());
    const core::WorkoutPlan generated = service_->generatePlan(*profile);
    ASSERT_FALSE(generated.isEmpty());
    (void)service_->saveGeneratedPlan(*profile, generated, "Saved Plan", false);

    const std::string output = run("1\n3\n1\n0\n4\n1\n0\n0\n");
    EXPECT_NE(output.find("Saved Plan"), std::string::npos) << output;
}

} // namespace
} // namespace workout::ui
