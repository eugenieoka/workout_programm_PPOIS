#include "core/User.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace workout::core {
namespace {

constexpr std::size_t kMaxNameLength = 100;
constexpr int kMinAge = 13;
constexpr int kMaxAge = 100;
constexpr double kMinHeightCm = 50.0;
constexpr double kMaxHeightCm = 250.0;
constexpr double kMinWeightKg = 20.0;
constexpr double kMaxWeightKg = 300.0;
constexpr double kMinBodyFatPct = 3.0;
constexpr double kMaxBodyFatPct = 50.0;
constexpr int kMinSessionMinutes = 10;
constexpr int kMaxSessionMinutes = 300;

bool isBlank(std::string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
}

bool isValidGender(Gender gender) noexcept {
    return static_cast<uint8_t>(gender) <= static_cast<uint8_t>(Gender::Other);
}

bool isValidLevel(ExperienceLevel level) noexcept {
    return static_cast<uint8_t>(level) <= static_cast<uint8_t>(ExperienceLevel::Advanced);
}

bool isValidGoal(TrainingGoal goal) noexcept {
    return static_cast<uint8_t>(goal) <= static_cast<uint8_t>(TrainingGoal::Maintenance);
}

bool isValidFrequency(TrainingFrequency frequency) noexcept {
    const int value = static_cast<int>(frequency);
    return value >= static_cast<int>(TrainingFrequency::Days2) &&
           value <= static_cast<int>(TrainingFrequency::Days6);
}

double bmrGenderOffset(Gender gender) noexcept {
    switch (gender) {
        case Gender::Male:   return 5.0;
        case Gender::Female: return -161.0;
        case Gender::Other:  return -78.0; // average of male (+5) and female (-161)
    }
    return 0.0;
}

double activityMultiplier(ExperienceLevel level, TrainingFrequency frequency) noexcept {
    static constexpr std::array<std::array<double, 5>, 3> kMultipliers{{
        {{1.35, 1.45, 1.55, 1.65, 1.70}}, // Novice
        {{1.40, 1.55, 1.65, 1.75, 1.80}}, // Intermediate
        {{1.45, 1.60, 1.70, 1.80, 1.85}}  // Advanced
    }};

    const int levelIndex = static_cast<int>(level);
    const int frequencyIndex =
        static_cast<int>(frequency) - static_cast<int>(TrainingFrequency::Days2);

    if (levelIndex < 0 || levelIndex >= static_cast<int>(kMultipliers.size()) ||
        frequencyIndex < 0 ||
        frequencyIndex >= static_cast<int>(kMultipliers[0].size())) {
        return 1.2; // sedentary fallback for corrupt database rows
    }

    return kMultipliers[levelIndex][frequencyIndex];
}

void validateOrThrow(const UserProfile& profile) {
    const BodyMetrics& metrics = profile.getBodyMetrics();
    const ValidationResult result = UserProfile::validate(
        profile.getName(),
        metrics.ageYears,
        profile.getGender(),
        metrics.heightCm,
        metrics.weightKg,
        profile.getLevel(),
        profile.getGoal(),
        profile.getFrequency(),
        profile.getConstraints(),
        metrics.bodyFatPct,
        metrics.trainingAgeMonths);

    if (!result.isValid) {
        throw std::invalid_argument(result.toString());
    }
}

} // namespace

std::string ValidationResult::toString() const {
    if (isValid) {
        return {};
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i != 0) {
            out << '\n';
        }
        if (i < errorCodes.size()) {
            out << errorCodes[i] << ": ";
        }
        out << errors[i];
    }
    return out.str();
}

void BodyMetrics::recalculateDerived(Gender gender,
                                    TrainingFrequency frequency,
                                    ExperienceLevel level) noexcept {
    bmi = 0.0;
    bmrKcal = 0.0;
    tdeeKcal = 0.0;
    ffmi = 0.0;

    if (!std::isfinite(heightCm) || !std::isfinite(weightKg) ||
        heightCm <= 0.0 || weightKg <= 0.0 || ageYears <= 0) {
        return;
    }

    const double heightM = heightCm / 100.0;
    const double heightSquared = heightM * heightM;

    bmi = weightKg / heightSquared;

    bmrKcal = 10.0 * weightKg
            + 6.25 * heightCm
            - 5.0 * static_cast<double>(ageYears)
            + bmrGenderOffset(gender);

    tdeeKcal = bmrKcal * activityMultiplier(level, frequency);

    if (std::isfinite(bodyFatPct) && bodyFatPct >= 0.0 && bodyFatPct < 100.0) {
        const double leanMassKg = weightKg * (1.0 - bodyFatPct / 100.0);
        ffmi = leanMassKg / heightSquared + 6.1 * (1.8 - heightM);
    }
}

ValidationResult UserProfile::validate(std::string_view name,
                                       int ageYears,
                                       Gender gender,
                                       double heightCm,
                                       double weightKg,
                                       ExperienceLevel level,
                                       TrainingGoal goal,
                                       TrainingFrequency frequency,
                                       const UserConstraints& constraints,
                                       double bodyFatPct,
                                       int trainingAgeMonths) {
    ValidationResult result;

    if (isBlank(name)) {
        result.addError("INVALID_NAME", "Name must not be empty or whitespace-only");
    } else if (name.size() > kMaxNameLength) {
        result.addError("NAME_TOO_LONG", "Name must be at most 100 bytes long");
    }

    if (ageYears < kMinAge || ageYears > kMaxAge) {
        result.addError("INVALID_AGE", "Age must be between 13 and 100");
    }

    if (!std::isfinite(heightCm) || heightCm <= kMinHeightCm || heightCm >= kMaxHeightCm) {
        result.addError("HEIGHT_OUT_OF_RANGE",
                        "Height must be greater than 50 cm and less than 250 cm");
    }

    if (!std::isfinite(weightKg) || weightKg <= kMinWeightKg || weightKg >= kMaxWeightKg) {
        result.addError("WEIGHT_OUT_OF_RANGE",
                        "Weight must be greater than 20 kg and less than 300 kg");
    }

    if (!std::isfinite(bodyFatPct) ||
        (bodyFatPct != -1.0 && (bodyFatPct < kMinBodyFatPct || bodyFatPct > kMaxBodyFatPct))) {
        result.addError("INVALID_BODY_FAT",
                        "Body fat must be -1 (unknown) or between 3 and 50 percent");
    }

    if (trainingAgeMonths < 0 ||
        (ageYears >= kMinAge && trainingAgeMonths > ageYears * 12)) {
        result.addError("INVALID_TRAINING_AGE",
                        "Training age must be between 0 and age * 12 months");
    }

    if (!isValidGender(gender)) {
        result.addError("INVALID_GENDER", "Gender value is out of range");
    }

    if (!isValidLevel(level)) {
        result.addError("INVALID_LEVEL", "Experience level value is out of range");
    }

    if (!isValidGoal(goal)) {
        result.addError("INVALID_GOAL", "Training goal value is out of range");
    }

    if (!isValidFrequency(frequency)) {
        result.addError("INVALID_FREQUENCY", "Training frequency value is out of range");
    }

    if (constraints.maxSessionMinutes &&
        (*constraints.maxSessionMinutes < kMinSessionMinutes ||
         *constraints.maxSessionMinutes > kMaxSessionMinutes)) {
        result.addError("INVALID_MAX_SESSION",
                        "Max session duration must be between 10 and 300 minutes");
    }

    return result;
}

std::optional<UserProfile> UserProfile::create(std::string name,
                                               int ageYears,
                                               Gender gender,
                                               double heightCm,
                                               double weightKg,
                                               ExperienceLevel level,
                                               TrainingGoal goal,
                                               TrainingFrequency frequency,
                                               UserConstraints constraints,
                                               double bodyFatPct,
                                               int trainingAgeMonths) {
    const ValidationResult validation = validate(name,
                                                ageYears,
                                                gender,
                                                heightCm,
                                                weightKg,
                                                level,
                                                goal,
                                                frequency,
                                                constraints,
                                                bodyFatPct,
                                                trainingAgeMonths);
    if (!validation.isValid) {
        return std::nullopt;
    }

    BodyMetrics metrics;
    metrics.heightCm = heightCm;
    metrics.weightKg = weightKg;
    metrics.bodyFatPct = bodyFatPct;
    metrics.ageYears = ageYears;
    metrics.trainingAgeMonths = trainingAgeMonths;
    metrics.recalculateDerived(gender, frequency, level);

    const auto now = std::chrono::system_clock::now();
    return UserProfile(-1,
                       std::move(name),
                       gender,
                       metrics,
                       level,
                       goal,
                       frequency,
                       std::move(constraints),
                       now,
                       now);
}

UserProfile UserProfile::createFromDb(int64_t id,
                                      std::string name,
                                      Gender gender,
                                      BodyMetrics metrics,
                                      ExperienceLevel level,
                                      TrainingGoal goal,
                                      TrainingFrequency frequency,
                                      UserConstraints constraints,
                                      std::chrono::system_clock::time_point createdAt,
                                      std::chrono::system_clock::time_point updatedAt) {
    if (id < 0) {
        throw std::invalid_argument("UserProfile::createFromDb: id must be non-negative");
    }

    return UserProfile(id,
                       std::move(name),
                       gender,
                       std::move(metrics),
                       level,
                       goal,
                       frequency,
                       std::move(constraints),
                       createdAt,
                       updatedAt);
}

UserProfile::UserProfile(int64_t id,
                        std::string name,
                        Gender gender,
                        BodyMetrics metrics,
                        ExperienceLevel level,
                        TrainingGoal goal,
                        TrainingFrequency freq,
                        UserConstraints constraints,
                        std::chrono::system_clock::time_point created,
                        std::chrono::system_clock::time_point updated)
    : id_(id),
      name_(std::move(name)),
      gender_(gender),
      bodyMetrics_(std::move(metrics)),
      level_(level),
      goal_(goal),
      frequency_(freq),
      constraints_(std::move(constraints)),
      createdAt_(created),
      updatedAt_(updated) {}

double UserProfile::getEstimated1RM(ExerciseId exerciseId) const noexcept {
    const auto it = estimated1RM_.find(exerciseId);
    return it == estimated1RM_.end() ? 0.0 : it->second;
}

UserProfile UserProfile::withBodyWeight(double newWeightKg) const {
    UserProfile updated = *this;
    updated.bodyMetrics_.weightKg = newWeightKg;
    validateOrThrow(updated);
    updated.bodyMetrics_.recalculateDerived(updated.gender_, updated.frequency_, updated.level_);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withBodyFatPct(double newBodyFatPct) const {
    UserProfile updated = *this;
    updated.bodyMetrics_.bodyFatPct = newBodyFatPct;
    validateOrThrow(updated);
    updated.bodyMetrics_.recalculateDerived(updated.gender_, updated.frequency_, updated.level_);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withTrainingAgeMonths(int months) const {
    UserProfile updated = *this;
    updated.bodyMetrics_.trainingAgeMonths = months;
    validateOrThrow(updated);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withLevel(ExperienceLevel newLevel) const {
    UserProfile updated = *this;
    updated.level_ = newLevel;
    validateOrThrow(updated);
    updated.bodyMetrics_.recalculateDerived(updated.gender_, updated.frequency_, updated.level_);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withGoal(TrainingGoal newGoal) const {
    UserProfile updated = *this;
    updated.goal_ = newGoal;
    validateOrThrow(updated);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withFrequency(TrainingFrequency newFreq) const {
    UserProfile updated = *this;
    updated.frequency_ = newFreq;
    validateOrThrow(updated);
    updated.bodyMetrics_.recalculateDerived(updated.gender_, updated.frequency_, updated.level_);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withConstraints(const UserConstraints& newConstraints) const {
    UserProfile updated = *this;
    updated.constraints_ = newConstraints;
    validateOrThrow(updated);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withName(std::string newName) const {
    UserProfile updated = *this;
    updated.name_ = std::move(newName);
    validateOrThrow(updated);
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

UserProfile UserProfile::withEstimated1RM(ExerciseId exerciseId, double kg) const {
    if (exerciseId <= 0) {
        throw std::invalid_argument(
            "UserProfile::withEstimated1RM: exercise id must be positive");
    }
    if (!std::isfinite(kg)) {
        throw std::invalid_argument(
            "UserProfile::withEstimated1RM: weight must be finite");
    }

    UserProfile updated = *this;
    if (kg > 0.0) {
        updated.estimated1RM_[exerciseId] = kg;
    } else {
        updated.estimated1RM_.erase(exerciseId);
    }
    updated.updatedAt_ = std::chrono::system_clock::now();
    return updated;
}

double UserProfile::getTargetCalories() const noexcept {
    if (!(bodyMetrics_.tdeeKcal > 0.0)) {
        return 0.0;
    }

    double delta = 0.0;
    switch (goal_) {
        case TrainingGoal::Strength:    delta = config::CALORIE_DELTA_STRENGTH; break;
        case TrainingGoal::Hypertrophy: delta = config::CALORIE_DELTA_HYPERTROPHY; break;
        case TrainingGoal::WeightLoss:  delta = config::CALORIE_DELTA_WEIGHT_LOSS; break;
        case TrainingGoal::Maintenance: delta = config::CALORIE_DELTA_MAINTENANCE; break;
    }

    return std::max(0.0, bodyMetrics_.tdeeKcal + delta);
}

} 
