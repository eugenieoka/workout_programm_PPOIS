#include "app/WorkoutService.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "app/PlanMapper.h"
#include "app/ServiceError.h"
#include "core/PlanGenerator.h"
#include "db/SqlTime.h"

namespace workout::app {
namespace {

constexpr int kMinSets = 1;
constexpr int kMaxSets = 20;
constexpr int kMinReps = 1;
constexpr int kMaxReps = 100;
constexpr int kMinRestSec = 0;
constexpr int kMaxRestSec = 3600;

void validateExerciseParameters(int sets, int reps, int restSec,
                                const std::optional<double>& weightKg) {
    if (sets < kMinSets || sets > kMaxSets) {
        throw std::invalid_argument("sets must be between 1 and 20");
    }
    if (reps < kMinReps || reps > kMaxReps) {
        throw std::invalid_argument("reps must be between 1 and 100");
    }
    if (restSec < kMinRestSec || restSec > kMaxRestSec) {
        throw std::invalid_argument("restSec must be between 0 and 3600");
    }
    if (weightKg && (!std::isfinite(*weightKg) || *weightKg < 0.0)) {
        throw std::invalid_argument("weightKg must be finite and non-negative");
    }
}

} // namespace

WorkoutService::WorkoutService(infra::Database& db)
    : db_(db), users_(db), exercises_(db), plans_(db) {}

std::vector<core::UserProfile> WorkoutService::listProfiles() {
    return users_.findAll();
}

std::optional<core::UserProfile> WorkoutService::getProfile(int64_t id) {
    return users_.findById(id);
}

int64_t WorkoutService::saveProfile(const core::UserProfile& profile) {
    return users_.save(profile);
}

bool WorkoutService::deleteProfile(int64_t id) {
    return users_.remove(id);
}

std::map<core::ExerciseId, double> WorkoutService::getOneRepMaxes(int64_t userId) {
    return requireProfile(userId).getEstimated1RM();
}

void WorkoutService::setOneRepMax(int64_t userId, core::ExerciseId exerciseId, double kg) {
    if (exerciseId <= 0) {
        throw std::invalid_argument("exercise id must be positive");
    }
    if (!std::isfinite(kg) || kg <= 0.0) {
        throw std::invalid_argument("one-rep max must be finite and positive");
    }
    if (!exercises_.findById(exerciseId)) {
        throw ServiceError("EXERCISE_NOT_FOUND",
                           "Exercise " + std::to_string(exerciseId) + " not found");
    }

    const core::UserProfile profile = requireProfile(userId);
    users_.save(profile.withEstimated1RM(exerciseId, kg));
}

void WorkoutService::removeOneRepMax(int64_t userId, core::ExerciseId exerciseId) {
    if (exerciseId <= 0) {
        throw std::invalid_argument("exercise id must be positive");
    }
    if (!exercises_.findById(exerciseId)) {
        throw ServiceError("EXERCISE_NOT_FOUND",
                           "Exercise " + std::to_string(exerciseId) + " not found");
    }

    const core::UserProfile profile = requireProfile(userId);
    users_.save(profile.withEstimated1RM(exerciseId, 0.0));
}

std::vector<core::Exercise> WorkoutService::listExercises(const ExerciseFilter& filter) {
    const std::vector<core::Exercise> all = exercises_.findAll();
    std::vector<core::Exercise> result;
    result.reserve(all.size());

    for (const core::Exercise& exercise : all) {
        if (filter.matches(exercise)) {
            result.push_back(exercise);
        }
    }
    return result;
}

std::optional<core::Exercise> WorkoutService::getExercise(core::ExerciseId id) {
    return exercises_.findById(id);
}

std::vector<core::Exercise> WorkoutService::findCompatible(
    const core::UserConstraints& constraints) {
    return exercises_.findCompatible(constraints);
}

core::WorkoutPlan WorkoutService::generatePlan(const core::UserProfile& profile) {
    const std::vector<core::Exercise> catalog =
        exercises_.findCompatible(profile.getConstraints());
    return core::PlanGenerator::generate(profile, catalog);
}

std::vector<PlanInfo> WorkoutService::listPlans(int64_t userId) {
    const auto names = exerciseNames();
    const std::vector<infra::PlanRecord> records = plans_.findByUser(userId);

    std::vector<PlanInfo> result;
    result.reserve(records.size());
    for (const infra::PlanRecord& record : records) {
        result.push_back(PlanMapper::toPlanInfo(record, names));
    }
    return result;
}

std::optional<PlanInfo> WorkoutService::getPlan(int64_t planId) {
    const std::optional<infra::PlanRecord> record = plans_.findById(planId);
    if (!record) {
        return std::nullopt;
    }
    return PlanMapper::toPlanInfo(*record, exerciseNames());
}

int64_t WorkoutService::saveGeneratedPlan(const core::UserProfile& profile,
                                          const core::WorkoutPlan& plan,
                                          const std::string& name,
                                          bool isActive) {
    if (profile.getId() <= 0) {
        throw ServiceError("INVALID_ARGUMENT",
                           "Profile must be saved before saving a generated plan");
    }
    if (plan.isEmpty()) {
        throw ServiceError("EMPTY_PLAN", "Generated plan has no exercises");
    }
    if (name.empty()) {
        throw ServiceError("INVALID_ARGUMENT", "Plan name must not be empty");
    }

    const infra::PlanRecord record = PlanMapper::fromGeneratedPlan(
        plan, profile.getId(), name, profile.getGoal(),
        static_cast<int>(profile.getFrequency()), isActive);
    return plans_.save(record);
}

bool WorkoutService::updatePlanExercise(int64_t planId, int dayIndex, int position,
                                        int sets, int reps, int restSec,
                                        std::optional<double> weightKg) {
    validateExerciseParameters(sets, reps, restSec, weightKg);

    std::optional<infra::PlanRecord> record = plans_.findById(planId);
    if (!record) {
        return false;
    }

    const auto dayIt = std::find_if(record->days.begin(), record->days.end(),
                                    [dayIndex](const infra::PlanDayRecord& day) {
                                        return day.dayIndex == dayIndex;
                                    });
    if (dayIt == record->days.end()) {
        return false;
    }

    const auto exerciseIt = std::find_if(
        dayIt->exercises.begin(), dayIt->exercises.end(),
        [position](const infra::PlanExerciseRecord& exercise) {
            return exercise.position == position;
        });
    if (exerciseIt == dayIt->exercises.end()) {
        return false;
    }

    exerciseIt->sets = sets;
    exerciseIt->reps = reps;
    exerciseIt->restSec = restSec;
    exerciseIt->weightKg = weightKg;

    // PlanRepository::save() replaces days and exercises as a whole. Keep the
    // active flag and force updated_at so the edit is visible to the UI.
    record->updatedAt = infra::sql::nowIso();
    plans_.save(*record);
    return true;
}

bool WorkoutService::activatePlan(int64_t planId) {
    return plans_.setActive(planId);
}

bool WorkoutService::deletePlan(int64_t planId) {
    return plans_.remove(planId);
}

std::map<core::ExerciseId, std::string> WorkoutService::exerciseNames() {
    std::map<core::ExerciseId, std::string> names;
    for (const core::Exercise& exercise : exercises_.findAll()) {
        names.emplace(exercise.getId(), exercise.getName());
    }
    return names;
}

core::UserProfile WorkoutService::requireProfile(int64_t userId) {
    const std::optional<core::UserProfile> profile = users_.findById(userId);
    if (!profile) {
        throw ServiceError("PROFILE_NOT_FOUND",
                           "Profile " + std::to_string(userId) + " not found");
    }
    return *profile;
}

} // namespace workout::app
