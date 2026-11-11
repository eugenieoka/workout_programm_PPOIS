#include "core/PlanGenerator.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace workout::core {
namespace {

constexpr int kDefaultMaxSessionMinutes = 90;
constexpr std::size_t kMaxExercisesPerDay = 6;

enum class DayType {
    FullBody,
    Upper,
    Lower,
    Push,
    Pull,
    Legs
};

struct DayTemplate {
    const char* name;
    DayType type;
};

struct Prescription {
    int sets;
    int reps;
    int restSec;
};

[[nodiscard]] Prescription prescriptionFor(TrainingGoal goal,
                                           ExerciseStructure structure) noexcept {
    switch (goal) {
        case TrainingGoal::Strength:
            return {4, 5, 270};
        case TrainingGoal::Hypertrophy:
            return {structure == ExerciseStructure::Compound ? 4 : 3, 10, 135};
        case TrainingGoal::WeightLoss:
            return {3, 12, 90};
        case TrainingGoal::Maintenance:
            return {3, 10, 135};
    }
    return {3, 10, 135};
}

[[nodiscard]] std::vector<DayTemplate> splitFor(TrainingFrequency frequency) {
    switch (frequency) {
        case TrainingFrequency::Days2:
            return {{"Full Body A", DayType::FullBody},
                    {"Full Body B", DayType::FullBody}};
        case TrainingFrequency::Days3:
            return {{"Full Body A", DayType::FullBody},
                    {"Full Body B", DayType::FullBody},
                    {"Full Body C", DayType::FullBody}};
        case TrainingFrequency::Days4:
            return {{"Upper A", DayType::Upper},
                    {"Lower A", DayType::Lower},
                    {"Upper B", DayType::Upper},
                    {"Lower B", DayType::Lower}};
        case TrainingFrequency::Days5:
            return {{"Push", DayType::Push},
                    {"Pull", DayType::Pull},
                    {"Legs", DayType::Legs},
                    {"Upper", DayType::Upper},
                    {"Lower", DayType::Lower}};
        case TrainingFrequency::Days6:
            return {{"Push A", DayType::Push},
                    {"Pull A", DayType::Pull},
                    {"Legs A", DayType::Legs},
                    {"Push B", DayType::Push},
                    {"Pull B", DayType::Pull},
                    {"Legs B", DayType::Legs}};
    }
    return {};
}

[[nodiscard]] std::vector<MajorMuscleGroup> targetsFor(DayType type) {
    switch (type) {
        case DayType::FullBody:
            return {MajorMuscleGroup::Legs,
                    MajorMuscleGroup::Chest,
                    MajorMuscleGroup::Back,
                    MajorMuscleGroup::Shoulders,
                    MajorMuscleGroup::Core,
                    MajorMuscleGroup::Arms};
        case DayType::Upper:
            return {MajorMuscleGroup::Chest,
                    MajorMuscleGroup::Back,
                    MajorMuscleGroup::Shoulders,
                    MajorMuscleGroup::Arms};
        case DayType::Lower:
            return {MajorMuscleGroup::Legs,
                    MajorMuscleGroup::Core};
        case DayType::Push:
            return {MajorMuscleGroup::Chest,
                    MajorMuscleGroup::Shoulders,
                    MajorMuscleGroup::Arms};
        case DayType::Pull:
            return {MajorMuscleGroup::Back,
                    MajorMuscleGroup::Arms};
        case DayType::Legs:
            return {MajorMuscleGroup::Legs,
                    MajorMuscleGroup::Core};
    }
    return {};
}

[[nodiscard]] int purposeRank(PurposeType purpose) noexcept {
    switch (purpose) {
        case PurposeType::Main:
            return 0;
        case PurposeType::Accessory:
            return 1;
        case PurposeType::Corrective:
            return 2;
        case PurposeType::Conditioning:
            return 3;
        case PurposeType::Warmup:
            return 4;
    }
    return 4;
}

[[nodiscard]] bool isEligible(const Exercise& exercise,
                              const UserConstraints& constraints) noexcept {
    if (exercise.getId() <= 0) {
        return false;
    }

    if (exercise.getKind() != ExerciseKind::Resistance &&
        exercise.getKind() != ExerciseKind::Olympic) {
        return false;
    }

    if (exercise.getPurposeType() == PurposeType::Warmup) {
        return false;
    }

    return exercise.isCompatibleWith(constraints);
}

[[nodiscard]] bool isBetter(const Exercise& lhs,
                            const Exercise& rhs,
                            const UserConstraints& constraints) noexcept {
    if (lhs.getStructure() != rhs.getStructure()) {
        return lhs.getStructure() < rhs.getStructure();
    }

    if (lhs.unilateralPenalty(constraints) != rhs.unilateralPenalty(constraints)) {
        return lhs.unilateralPenalty(constraints) < rhs.unilateralPenalty(constraints);
    }

    if (purposeRank(lhs.getPurposeType()) != purposeRank(rhs.getPurposeType())) {
        return purposeRank(lhs.getPurposeType()) < purposeRank(rhs.getPurposeType());
    }

    if (lhs.getDifficulty() != rhs.getDifficulty()) {
        return lhs.getDifficulty() < rhs.getDifficulty();
    }

    if (lhs.getId() != rhs.getId()) {
        return lhs.getId() < rhs.getId();
    }

    return lhs.getName() < rhs.getName();
}

[[nodiscard]] std::vector<const Exercise*> buildPool(
    const std::vector<Exercise>& catalog,
    const UserConstraints& constraints) {
    std::vector<const Exercise*> pool;
    pool.reserve(catalog.size());

    for (const Exercise& exercise : catalog) {
        if (isEligible(exercise, constraints)) {
            pool.push_back(&exercise);
        }
    }

    std::stable_sort(pool.begin(), pool.end(),
                     [&constraints](const Exercise* lhs, const Exercise* rhs) {
                         return isBetter(*lhs, *rhs, constraints);
                     });

    return pool;
}

[[nodiscard]] int estimatedExerciseSeconds(const Exercise& exercise,
                                           const Prescription& prescription) noexcept {
    return prescription.sets *
           (exercise.getEstimatedSetDurationSec() + prescription.restSec);
}

[[nodiscard]] std::vector<PlannedExercise> buildDay(
    const std::vector<const Exercise*>& pool,
    const std::vector<MajorMuscleGroup>& targets,
    int maxSessionSeconds,
    TrainingGoal goal,
    const UserConstraints& constraints,
    std::set<ExerciseId>& weekUsed) {
    std::vector<const Exercise*> selected;
    std::set<ExerciseId> dayUsed;
    int elapsedSeconds = 0;

    const auto add = [&](const Exercise* exercise) {
        const Prescription prescription = prescriptionFor(goal, exercise->getStructure());
        selected.push_back(exercise);
        dayUsed.insert(exercise->getId());
        weekUsed.insert(exercise->getId());
        elapsedSeconds += estimatedExerciseSeconds(*exercise, prescription);
    };

    const auto pick = [&](MajorMuscleGroup target,
                          bool allowWeekUsed) -> const Exercise* {
        for (const Exercise* exercise : pool) {
            if (dayUsed.count(exercise->getId()) != 0) {
                continue;
            }

            if (!allowWeekUsed && weekUsed.count(exercise->getId()) != 0) {
                continue;
            }

            if (toMajorGroup(exercise->getPrimaryMuscle()) != target) {
                continue;
            }

            const Prescription prescription =
                prescriptionFor(goal, exercise->getStructure());
            if (estimatedExerciseSeconds(*exercise, prescription) >
                maxSessionSeconds - elapsedSeconds) {
                continue;
            }

            return exercise;
        }
        return nullptr;
    };

    const auto pickForTargets = [&](bool allowWeekUsed) -> const Exercise* {
        for (const Exercise* exercise : pool) {
            if (dayUsed.count(exercise->getId()) != 0) {
                continue;
            }

            if (!allowWeekUsed && weekUsed.count(exercise->getId()) != 0) {
                continue;
            }

            const MajorMuscleGroup major =
                toMajorGroup(exercise->getPrimaryMuscle());
            if (std::find(targets.begin(), targets.end(), major) == targets.end()) {
                continue;
            }

            const Prescription prescription =
                prescriptionFor(goal, exercise->getStructure());
            if (estimatedExerciseSeconds(*exercise, prescription) >
                maxSessionSeconds - elapsedSeconds) {
                continue;
            }

            return exercise;
        }
        return nullptr;
    };

    for (const MajorMuscleGroup target : targets) {
        if (selected.size() >= kMaxExercisesPerDay) {
            break;
        }

        if (const Exercise* exercise = pick(target, false)) {
            add(exercise);
        }
    }

    while (selected.size() < kMaxExercisesPerDay) {
        const Exercise* exercise = pickForTargets(false);
        if (exercise == nullptr) {
            exercise = pickForTargets(true);
        }

        if (exercise == nullptr) {
            break;
        }
        add(exercise);
    }

    std::stable_sort(selected.begin(), selected.end(),
                     [&constraints](const Exercise* lhs, const Exercise* rhs) {
                         return isBetter(*lhs, *rhs, constraints);
                     });

    std::vector<PlannedExercise> result;
    result.reserve(selected.size());
    for (const Exercise* exercise : selected) {
        const Prescription prescription = prescriptionFor(goal, exercise->getStructure());
        result.push_back({exercise->getId(), prescription.sets,
                          prescription.reps, prescription.restSec});
    }
    return result;
}

} // namespace

WorkoutPlan PlanGenerator::generate(const UserProfile& user,
                                    const std::vector<Exercise>& catalog) {
    const UserConstraints& constraints = user.getConstraints();
    const std::vector<const Exercise*> pool = buildPool(catalog, constraints);

    if (pool.empty()) {
        return WorkoutPlan{};
    }

    const std::vector<DayTemplate> split = splitFor(user.getFrequency());
    const int maxSessionMinutes =
        constraints.maxSessionMinutes.value_or(kDefaultMaxSessionMinutes);
    const int maxSessionSeconds = maxSessionMinutes * 60;

    std::vector<PlanDay> days;
    days.reserve(split.size());

    std::set<ExerciseId> weekUsed;
    bool hasAnyExercise = false;

    for (std::size_t index = 0; index < split.size(); ++index) {
        PlanDay day;
        day.dayIndex = static_cast<int>(index) + 1;
        day.name = split[index].name;
        day.exercises = buildDay(pool, targetsFor(split[index].type),
                                 maxSessionSeconds, user.getGoal(), constraints,
                                 weekUsed);

        hasAnyExercise = hasAnyExercise || !day.exercises.empty();
        days.push_back(std::move(day));
    }

    if (!hasAnyExercise) {
        return WorkoutPlan{};
    }

    return WorkoutPlan{std::move(days)};
}

} // namespace workout::core
