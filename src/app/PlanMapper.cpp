#include "app/PlanMapper.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace workout::app {
namespace {

constexpr int kMinSets = 1;
constexpr int kMaxSets = 20;
constexpr int kMinReps = 1;
constexpr int kMaxReps = 100;
constexpr int kMinRestSec = 0;
constexpr int kMaxRestSec = 3600;

void validateExercise(const core::PlannedExercise& exercise, std::size_t dayIndex,
                      std::size_t position) {
    const std::string where = "day " + std::to_string(dayIndex) +
                              ", exercise " + std::to_string(position);
    if (exercise.exerciseId <= 0) {
        throw std::invalid_argument("PlanMapper: exercise id must be positive at " + where);
    }
    if (exercise.sets < kMinSets || exercise.sets > kMaxSets) {
        throw std::invalid_argument("PlanMapper: sets must be between 1 and 20 at " + where);
    }
    if (exercise.reps < kMinReps || exercise.reps > kMaxReps) {
        throw std::invalid_argument("PlanMapper: reps must be between 1 and 100 at " + where);
    }
    if (exercise.restSec < kMinRestSec || exercise.restSec > kMaxRestSec) {
        throw std::invalid_argument("PlanMapper: restSec must be between 0 and 3600 at " + where);
    }
}

} // namespace

infra::PlanRecord PlanMapper::fromGeneratedPlan(const core::WorkoutPlan& plan,
                                                 int64_t userId,
                                                 const std::string& name,
                                                 core::TrainingGoal goal,
                                                 int daysPerWeek,
                                                 bool isActive) {
    if (userId <= 0) {
        throw std::invalid_argument("PlanMapper: user id must be positive");
    }
    if (name.empty()) {
        throw std::invalid_argument("PlanMapper: plan name must not be empty");
    }
    if (plan.isEmpty()) {
        throw std::invalid_argument("PlanMapper: plan must contain at least one day");
    }

    const auto& days = plan.getDays();
    if (daysPerWeek <= 0) {
        daysPerWeek = static_cast<int>(days.size());
    }
    if (daysPerWeek < 1 || daysPerWeek > 7) {
        throw std::invalid_argument("PlanMapper: daysPerWeek must be between 1 and 7");
    }

    infra::PlanRecord record;
    record.userId = userId;
    record.name = name;
    record.goal = goal;
    record.daysPerWeek = daysPerWeek;
    record.isActive = isActive;
    record.days.reserve(days.size());

    for (std::size_t dayIndex = 0; dayIndex < days.size(); ++dayIndex) {
        const core::PlanDay& day = days[dayIndex];
        infra::PlanDayRecord dayRecord;
        dayRecord.dayIndex = static_cast<int>(dayIndex); // zero-based by contract
        dayRecord.name = day.name;
        dayRecord.exercises.reserve(day.exercises.size());

        for (std::size_t position = 0; position < day.exercises.size(); ++position) {
            const core::PlannedExercise& exercise = day.exercises[position];
            validateExercise(exercise, dayIndex, position);

            infra::PlanExerciseRecord exerciseRecord;
            exerciseRecord.exerciseId = exercise.exerciseId;
            exerciseRecord.position = static_cast<int>(position); // zero-based by contract
            exerciseRecord.sets = exercise.sets;
            exerciseRecord.reps = exercise.reps;
            exerciseRecord.restSec = exercise.restSec;
            exerciseRecord.weightKg = std::nullopt;
            exerciseRecord.notes.clear();
            dayRecord.exercises.push_back(std::move(exerciseRecord));
        }

        record.days.push_back(std::move(dayRecord));
    }

    return record;
}

PlanInfo PlanMapper::toPlanInfo(
    const infra::PlanRecord& record,
    const std::map<core::ExerciseId, std::string>& exerciseNames) {
    PlanInfo info;
    info.id = record.id;
    info.userId = record.userId;
    info.name = record.name;
    info.goal = record.goal;
    info.daysPerWeek = record.daysPerWeek > 0
                           ? record.daysPerWeek
                           : static_cast<int>(record.days.size());
    info.isActive = record.isActive;
    info.notes = record.notes;
    info.createdAt = record.createdAt;
    info.updatedAt = record.updatedAt;
    info.days.reserve(record.days.size());

    for (std::size_t dayIndex = 0; dayIndex < record.days.size(); ++dayIndex) {
        const infra::PlanDayRecord& day = record.days[dayIndex];
        PlanDayInfo dayInfo;
        dayInfo.dayIndex = day.dayIndex >= 0 ? day.dayIndex : static_cast<int>(dayIndex);
        dayInfo.name = day.name;
        dayInfo.exercises.reserve(day.exercises.size());

        for (std::size_t position = 0; position < day.exercises.size(); ++position) {
            const infra::PlanExerciseRecord& exercise = day.exercises[position];
            PlanExerciseInfo exerciseInfo;
            exerciseInfo.position =
                exercise.position >= 0 ? exercise.position : static_cast<int>(position);
            exerciseInfo.exerciseId = exercise.exerciseId;
            const auto nameIt = exerciseNames.find(exercise.exerciseId);
            exerciseInfo.exerciseName = nameIt != exerciseNames.end()
                                            ? nameIt->second
                                            : "#" + std::to_string(exercise.exerciseId);
            exerciseInfo.sets = exercise.sets;
            exerciseInfo.reps = exercise.reps;
            exerciseInfo.restSec = exercise.restSec;
            exerciseInfo.weightKg = exercise.weightKg;
            exerciseInfo.notes = exercise.notes;
            dayInfo.exercises.push_back(std::move(exerciseInfo));
        }

        std::stable_sort(dayInfo.exercises.begin(), dayInfo.exercises.end(),
                         [](const PlanExerciseInfo& lhs, const PlanExerciseInfo& rhs) {
                             return lhs.position < rhs.position;
                         });
        info.days.push_back(std::move(dayInfo));
    }

    std::stable_sort(info.days.begin(), info.days.end(),
                     [](const PlanDayInfo& lhs, const PlanDayInfo& rhs) {
                         return lhs.dayIndex < rhs.dayIndex;
                     });

    return info;
}

} // namespace workout::app
