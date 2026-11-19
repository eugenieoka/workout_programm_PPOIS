#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "app/ServiceError.h"
#include "app/WorkoutService.h"

namespace workout::ui {

/**
 * @brief Menu-driven Russian console UI.
 *
 * The class owns all console-specific formatting and input handling. Domain
 * and database logic stay in WorkoutService.
 */
class ConsoleUI {
public:
    ConsoleUI(app::WorkoutService& service, std::istream& in, std::ostream& out);

    void run();

private:
    void mainMenu();
    void profileMenu();
    void catalogMenu();
    void plansMenu();

    void showProfiles();
    void createProfile();
    void selectProfile();
    void editProfile();
    void manageOneRepMaxes();
    void deleteProfile();

    void showCatalog(const app::ExerciseFilter& filter);
    void showExerciseDetails();

    void generatePlan();
    void showPlans();
    void viewPlan();
    void activatePlan();
    void deletePlan();

    [[nodiscard]] std::optional<core::UserProfile> selectedProfile();
    [[nodiscard]] std::map<core::ExerciseId, std::string> exerciseNames();

    [[nodiscard]] core::Gender askGender(core::Gender current);
    [[nodiscard]] core::ExperienceLevel askLevel(core::ExperienceLevel current);
    [[nodiscard]] core::TrainingGoal askGoal(core::TrainingGoal current);
    [[nodiscard]] core::TrainingFrequency askFrequency(
        core::TrainingFrequency current);
    [[nodiscard]] core::Equipment askEquipment(core::Equipment current);
    [[nodiscard]] std::uint16_t askHealthMask(std::uint16_t current);
    [[nodiscard]] std::optional<int> askMaxSession(
        const std::optional<int>& current);
    [[nodiscard]] core::UserConstraints askConstraints(
        const core::UserConstraints& current);

    [[nodiscard]] std::string readLine(std::string_view prompt);
    [[nodiscard]] std::string readNonEmptyLine(std::string_view prompt);
    [[nodiscard]] int readInt(std::string_view prompt, int min, int max);
    [[nodiscard]] std::optional<int> readOptionalInt(std::string_view prompt,
                                                     int min,
                                                     int max);
    [[nodiscard]] double readDouble(std::string_view prompt, double min, double max);
    [[nodiscard]] std::optional<double> readOptionalDouble(std::string_view prompt,
                                                           double min,
                                                           double max);
    [[nodiscard]] bool readYesNo(std::string_view prompt);

    void printError(const std::string& message);
    void printServiceError(const app::ServiceError& error);

    app::WorkoutService& service_;
    std::istream& in_;
    std::ostream& out_;
    std::optional<int64_t> selectedProfileId_;
    bool running_ = true;
};

} // namespace workout::ui
