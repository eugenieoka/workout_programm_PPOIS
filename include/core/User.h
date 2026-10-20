#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <chrono>
#include <cstdint>
#include <map>
#include <array>

namespace workout::core {

using ExerciseId = int64_t;

struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
    std::vector<std::string> errorCodes;

    void addError(std::string_view code, std::string_view msg) {
        isValid = false;
        errorCodes.emplace_back(code);
        errors.emplace_back(msg);
    }
    [[nodiscard]] std::string toString() const;
};

// Enums

enum class Gender : uint8_t {
    Male = 0,
    Female = 1,
    Other = 2
};

enum class ExperienceLevel : uint8_t {
    Novice = 0,
    Intermediate = 1,
    Advanced = 2
};

enum class TrainingGoal : uint8_t {
    Strength = 0,
    Hypertrophy = 1,
    WeightLoss = 2,
    Maintenance = 3
};

enum class TrainingFrequency : uint8_t {
    Days2 = 2,
    Days3 = 3,
    Days4 = 4,
    Days5 = 5,
    Days6 = 6
};

enum class HealthConstraint : uint8_t {
    None = 0,
    LowerBackPain = 1,
    KneePain = 2,
    ShoulderImpingement = 3,
    ElbowTendinopathy = 4,
    WristPain = 5,
    HipImpingement = 6,
    CardiovascularIssue = 7,
    Hernia = 8,
    Custom = 9
};

constexpr uint16_t HC_MASK(HealthConstraint hc) noexcept {
    return hc == HealthConstraint::None ? 0u : (1u << static_cast<uint8_t>(hc));
}

enum class Equipment : uint16_t {
    None = 0,
    Barbell = 1 << 0,
    Dumbbells = 1 << 1,
    PowerRack = 1 << 2,
    CableMachine = 1 << 3,
    Machines = 1 << 4,
    PullUpBar = 1 << 5,
    DipStation = 1 << 6,
    Bands = 1 << 7,
    Kettlebells = 1 << 8,
    CardioMachines = 1 << 9
};
constexpr Equipment operator|(Equipment a, Equipment b) noexcept {
    return static_cast<Equipment>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
constexpr Equipment operator&(Equipment a, Equipment b) noexcept {
    return static_cast<Equipment>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}
constexpr bool hasEquipment(Equipment have, Equipment need) noexcept {
    return (static_cast<uint16_t>(have) & static_cast<uint16_t>(need)) == static_cast<uint16_t>(need);
}

// BodyMetrics

struct BodyMetrics {
    double heightCm = 0.0;
    double weightKg = 0.0;
    double bodyFatPct = -1.0;  
    int ageYears = 0;
    int trainingAgeMonths = 0;
    double bmi = 0.0;
    double bmrKcal = 0.0;
    double tdeeKcal = 0.0;
    double ffmi = 0.0;

    [[nodiscard]] bool isValid() const noexcept {
        constexpr double EPS = 1e-9;
        return heightCm > 50.0 && heightCm < 250.0 &&
            weightKg > 20.0 && weightKg < 300.0 &&
            ageYears >= 13 && ageYears <= 100 &&
            (bodyFatPct < -0.5 || (bodyFatPct >= 3.0 && bodyFatPct <= 50.0)) &&
            trainingAgeMonths >= 0;
    }

    void recalculateDerived(Gender gender, TrainingFrequency frequency, ExperienceLevel level) noexcept;
};

// UserConstraints

struct UserConstraints {
    uint16_t constraintsMask = 0;
    std::string customNotes;

    Equipment availableEquipment = Equipment::None;

    bool avoidAxialLoading = false;
    bool avoidSpinalFlexion = false;
    bool avoidOverheadMotion = false;
    bool avoidDeepKneeFlexio = false;
    bool preferUnilateral = false;

    std::optional<int> maxSessionMinutes;

    [[nodiscard]] bool hasConstraint(HealthConstraint hc) const noexcept {
        return (constraintsMask & HC_MASK(hc)) != 0;
    }
    void addConstraint(HealthConstraint hc) noexcept { constraintsMask |= HC_MASK(hc); }
    void removeConstraint(HealthConstraint hc) noexcept { constraintsMask &= ~HC_MASK(hc); }

    /** @return true if OHP and similar overhead movements should be excluded. */
    [[nodiscard]] bool excludesOverheadPress() const noexcept {
        return avoidAxialLoading || avoidOverheadMotion;
    }
};

// Config constants

namespace config {
    constexpr double CALORIE_DELTA_STRENGTH = +200.0;
    constexpr double CALORIE_DELTA_HYPERTROPHY = +300.0;
    constexpr double CALORIE_DELTA_WEIGHT_LOSS = -400.0;
    constexpr double CALORIE_DELTA_MAINTENANCE = 0.0;
}

// UserProfile - Aggregate Root

class UserProfile {
public:
    static ValidationResult validate(
        std::string_view name,
        int ageYears,
        Gender gender,
        double heightCm,
        double weightKg,
        ExperienceLevel level,
        TrainingGoal goal,
        TrainingFrequency frequency,
        const UserConstraints& constraints,
        double bodyFatPct,
        int trainingAgeMonths
    );

    //Factories
    /**
     * @brief Create validated profile with computed derived metrics.
     * @return std::nullopt if validation fails (see validate() for details).
     */
    static std::optional<UserProfile> create(
        std::string name,
        int ageYears,
        Gender gender,
        double heightCm,
        double weightKg,
        ExperienceLevel level,
        TrainingGoal goal,
        TrainingFrequency frequency,
        UserConstraints constraints = {},
        double bodyFatPct = -1.0,
        int trainingAgeMonths = 0
    );

    /**
     * @brief Reconstruct profile from persisted data (repository use).
     * @param id Database primary key (must be >= 0)
     * @param createdAt Original creation timestamp
     * @param updatedAt Last update timestamp
     * @note Skips validation - trusts DB integrity.
     */
    static UserProfile createFromDb(
        int64_t id,
        std::string name,
        Gender gender,
        BodyMetrics metrics,
        ExperienceLevel level,
        TrainingGoal goal,
        TrainingFrequency frequency,
        UserConstraints constraints,
        std::chrono::system_clock::time_point createdAt,
        std::chrono::system_clock::time_point updatedAt
    );

    UserProfile() = default;

    //Getters
    [[nodiscard]] int64_t                               getId() const noexcept { return id_; }
    [[nodiscard]] const std::string&                    getName() const noexcept { return name_; }
    [[nodiscard]] int                                   getAge() const noexcept { return bodyMetrics_.ageYears; }
    [[nodiscard]] int                                   getTrainingAgeMonths() const noexcept { return bodyMetrics_.trainingAgeMonths; }
    [[nodiscard]] Gender                                getGender() const noexcept { return gender_; }
    [[nodiscard]] const BodyMetrics&                    getBodyMetrics() const noexcept { return bodyMetrics_; }
    [[nodiscard]] ExperienceLevel                       getLevel() const noexcept { return level_; }
    [[nodiscard]] TrainingGoal                          getGoal() const noexcept { return goal_; }
    [[nodiscard]] TrainingFrequency                     getFrequency() const noexcept { return frequency_; }
    [[nodiscard]] const UserConstraints&                getConstraints() const noexcept { return constraints_; }
    [[nodiscard]] std::chrono::system_clock::time_point getCreatedAt() const noexcept { return createdAt_; }
    [[nodiscard]] std::chrono::system_clock::time_point getUpdatedAt() const noexcept { return updatedAt_; }

    /** @return Estimated 1RM for exercise (kg), 0 if unknown. */
    [[nodiscard]] double getEstimated1RM(ExerciseId exerciseId) const noexcept;

    //Value-semantics updates
    UserProfile withBodyWeight(double newWeightKg) const;
    UserProfile withBodyFatPct(double newBodyFatPct) const;
    UserProfile withTrainingAgeMonths(int months) const;
    UserProfile withLevel(ExperienceLevel newLevel) const;
    UserProfile withGoal(TrainingGoal newGoal) const;
    UserProfile withFrequency(TrainingFrequency newFreq) const;
    UserProfile withConstraints(const UserConstraints& newConstraints) const;
    UserProfile withName(std::string newName) const;
    UserProfile withEstimated1RM(ExerciseId exerciseId, double kg) const;

    //Helpers
    [[nodiscard]] double getTargetCalories() const noexcept;
    [[nodiscard]] bool isNovice() const noexcept { return level_ == ExperienceLevel::Novice; }

    //Serialization
    /**
     * @return JSON with:
     *   - enum values as strings: "Male", "Intermediate", "Hypertrophy"
     *   - timestamps ISO 8601 UTC: "2026-03-20T14:22:00Z"
     *   - keys PascalCase
     *   - Equipment as array of strings: ["Barbell", "Dumbbells"]
     *   - Constraints as array of strings: ["LowerBackPain", "KneePain"]
     */
    [[nodiscard]] std::string toJson() const;

    /** @throw std::runtime_error on parse/validation error. */
    static UserProfile fromJson(const std::string& json);

private:
    int64_t id_ = -1;
    std::string name_;
    Gender gender_ = Gender::Male;
    BodyMetrics bodyMetrics_;
    ExperienceLevel level_ = ExperienceLevel::Novice;
    TrainingGoal goal_ = TrainingGoal::Strength;
    TrainingFrequency frequency_ = TrainingFrequency::Days3;
    UserConstraints constraints_;
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point updatedAt_;
    std::map<ExerciseId, double> estimated1RM_;

    UserProfile(int64_t id, std::string name, Gender gender, BodyMetrics metrics,
            ExperienceLevel level, TrainingGoal goal, TrainingFrequency freq,
            UserConstraints constraints,
            std::chrono::system_clock::time_point created,
            std::chrono::system_clock::time_point updated);
};

} // namespace workout::core