#include "core/WorkoutPlan.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace workout::core {
namespace {

constexpr int kMinSets = 1;
constexpr int kMaxSets = 20;
constexpr int kMinReps = 1;
constexpr int kMaxReps = 100;
constexpr int kMinRestSec = 0;
constexpr int kMaxRestSec = 3600;

bool isBlank(std::string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
}

std::string dayLabel(std::size_t dayPosition) {
    return "Day at position " + std::to_string(dayPosition);
}

} // namespace

WorkoutPlan::WorkoutPlan(std::vector<PlanDay> days)
    : days_(std::move(days)) {}

ValidationResult WorkoutPlan::validate() const {
    ValidationResult result;
    std::set<int> dayIndexes;

    for (std::size_t dayPosition = 0; dayPosition < days_.size(); ++dayPosition) {
        const PlanDay& day = days_[dayPosition];
        const std::string label = dayLabel(dayPosition);

        if (day.dayIndex < 0) {
            result.addError("INVALID_DAY_INDEX",
                            label + ": DayIndex must be non-negative");
        } else if (!dayIndexes.insert(day.dayIndex).second) {
            result.addError("DUPLICATE_DAY_INDEX",
                            label + ": DayIndex values must be unique");
        }

        if (isBlank(day.name)) {
            result.addError("INVALID_DAY_NAME",
                            label + ": Name must not be empty or whitespace-only");
        }

        for (std::size_t exercisePosition = 0;
             exercisePosition < day.exercises.size(); ++exercisePosition) {
            const PlannedExercise& exercise = day.exercises[exercisePosition];
            const std::string exerciseLabel =
                label + ", exercise at position " + std::to_string(exercisePosition);

            if (exercise.exerciseId <= 0) {
                result.addError("INVALID_EXERCISE_ID",
                                exerciseLabel + ": ExerciseId must be positive");
            }
            if (exercise.sets < kMinSets || exercise.sets > kMaxSets) {
                result.addError("INVALID_SETS",
                                exerciseLabel + ": Sets must be between 1 and 20");
            }
            if (exercise.reps < kMinReps || exercise.reps > kMaxReps) {
                result.addError("INVALID_REPS",
                                exerciseLabel + ": Reps must be between 1 and 100");
            }
            if (exercise.restSec < kMinRestSec || exercise.restSec > kMaxRestSec) {
                result.addError("INVALID_REST_SEC",
                                exerciseLabel + ": RestSec must be between 0 and 3600");
            }
        }
    }

    return result;
}

} // namespace workout::core
