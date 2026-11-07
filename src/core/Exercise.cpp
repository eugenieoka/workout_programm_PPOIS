#include "core/Exercise.h"

#include "core/User.h"
#include "ExerciseValidation.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace workout::core {
namespace {

constexpr std::size_t kMaxNameLength = 100;
constexpr std::size_t kMaxDescriptionLength = 2000;
constexpr std::size_t kMaxInstructionsLength = 10000;
constexpr std::size_t kMaxVideoUrlLength = 500;

constexpr uint8_t kMinDifficulty = 1;
constexpr uint8_t kMaxDifficulty = 5;
constexpr double kMaxMet = 30.0;
constexpr int kMinSetDurationSec = 5;
constexpr int kMaxSetDurationSec = 1800;

constexpr int kNonPreferredUnilateralPenalty = 10;

constexpr Equipment kKnownEquipment =
    Equipment::Barbell | Equipment::Dumbbells | Equipment::PowerRack |
    Equipment::CableMachine | Equipment::Machines | Equipment::PullUpBar |
    Equipment::DipStation | Equipment::Bands | Equipment::Kettlebells |
    Equipment::CardioMachines | Equipment::Bench;

bool isBlank(std::string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
}

bool isValidMuscleGroup(MuscleGroup mg) noexcept {
    return static_cast<uint8_t>(mg) < static_cast<uint8_t>(MuscleGroup::Count);
}

bool isValidStructure(ExerciseStructure structure) noexcept {
    return static_cast<uint8_t>(structure) <= static_cast<uint8_t>(ExerciseStructure::Isolation);
}

bool isValidKind(ExerciseKind kind) noexcept {
    return static_cast<uint8_t>(kind) <= static_cast<uint8_t>(ExerciseKind::Mobility);
}

bool isValidPurpose(PurposeType purpose) noexcept {
    return static_cast<uint8_t>(purpose) <= static_cast<uint8_t>(PurposeType::Warmup);
}

bool isValidPattern(MovementPattern pattern) noexcept {
    return static_cast<uint8_t>(pattern) <= static_cast<uint8_t>(MovementPattern::Other);
}

bool hasUnknownEquipmentBits(Equipment mask) noexcept {
    const uint16_t bits = static_cast<uint16_t>(mask);
    const uint16_t known = static_cast<uint16_t>(kKnownEquipment);
    return (bits & static_cast<uint16_t>(~known)) != 0;
}

} // namespace

namespace detail {

std::string validateExerciseData(const ExerciseData& data) {
    std::ostringstream out;
    bool first = true;

    const auto addError = [&](std::string_view message) {
        if (!first) {
            out << '\n';
        }
        first = false;
        out << message;
    };

    if (data.id < -1) {
        addError("Id must be -1 (unsaved) or non-negative");
    }

    if (isBlank(data.name)) {
        addError("Name must not be empty or whitespace-only");
    } else if (data.name.size() > kMaxNameLength) {
        addError("Name must be at most 100 bytes long");
    }

    if (data.description.size() > kMaxDescriptionLength) {
        addError("Description must be at most 2000 bytes long");
    }

    if (data.instructions.size() > kMaxInstructionsLength) {
        addError("Instructions must be at most 10000 bytes long");
    }

    if (data.videoUrl.size() > kMaxVideoUrlLength) {
        addError("Video URL must be at most 500 bytes long");
    }

    if (!isValidMuscleGroup(data.primaryMuscle)) {
        addError("Primary muscle must be a valid MuscleGroup value");
    }

    bool invalidSecondary = false;
    for (const MuscleGroup muscle : data.secondaryMuscles) {
        if (!isValidMuscleGroup(muscle)) {
            addError("Secondary muscles contain an invalid MuscleGroup value");
            invalidSecondary = true;
            break;
        }
    }

    if (!invalidSecondary &&
        std::find(data.secondaryMuscles.begin(), data.secondaryMuscles.end(),
                  data.primaryMuscle) != data.secondaryMuscles.end()) {
        addError("Secondary muscles must not contain the primary muscle");
    }

    if (!invalidSecondary) {
        bool hasDuplicate = false;
        for (std::size_t i = 0; i < data.secondaryMuscles.size() && !hasDuplicate; ++i) {
            for (std::size_t j = i + 1; j < data.secondaryMuscles.size(); ++j) {
                if (data.secondaryMuscles[i] == data.secondaryMuscles[j]) {
                    addError("Secondary muscles must not contain duplicates");
                    hasDuplicate = true;
                    break;
                }
            }
        }
    }

    if (!isValidStructure(data.structure)) {
        addError("Structure value is out of range");
    }

    if (!isValidKind(data.kind)) {
        addError("Kind value is out of range");
    }

    if (!isValidPurpose(data.purposeType)) {
        addError("Purpose type value is out of range");
    }

    if (!isValidPattern(data.pattern)) {
        addError("Movement pattern value is out of range");
    }

    if (hasUnknownEquipmentBits(data.requiredEquipment)) {
        addError("Required equipment contains unknown bits");
    }

    if (hasUnknownEquipmentBits(data.optionalEquipment)) {
        addError("Optional equipment contains unknown bits");
    }

    if ((static_cast<uint16_t>(data.requiredEquipment) &
         static_cast<uint16_t>(data.optionalEquipment)) != 0) {
        addError("Optional equipment must not overlap with required equipment");
    }

    if (data.difficulty < kMinDifficulty || data.difficulty > kMaxDifficulty) {
        addError("Difficulty must be between 1 and 5");
    }

    if (!std::isfinite(data.met) || data.met < 0.0 || data.met > kMaxMet) {
        addError("MET must be finite and between 0 and 30");
    }

    if (data.estimatedSetDurationSec < kMinSetDurationSec ||
        data.estimatedSetDurationSec > kMaxSetDurationSec) {
        addError("Estimated set duration must be between 5 and 1800 seconds");
    }

    if (data.regressionId && *data.regressionId <= 0) {
        addError("Regression id must be positive");
    }

    if (data.progressionId && *data.progressionId <= 0) {
        addError("Progression id must be positive");
    }

    if (data.regressionId && data.id >= 0 && *data.regressionId == data.id) {
        addError("Regression id must not reference the exercise itself");
    }

    if (data.progressionId && data.id >= 0 && *data.progressionId == data.id) {
        addError("Progression id must not reference the exercise itself");
    }

    if (data.regressionId && data.progressionId &&
        *data.regressionId == *data.progressionId) {
        addError("Regression and progression ids must differ");
    }

    return first ? std::string{} : out.str();
}

} // namespace detail

const char* toString(MuscleGroup mg) noexcept {
    switch (mg) {
        case MuscleGroup::Chest_Upper:     return "Chest_Upper";
        case MuscleGroup::Chest_Lower:     return "Chest_Lower";
        case MuscleGroup::Back_Lats:       return "Back_Lats";
        case MuscleGroup::Back_Traps:      return "Back_Traps";
        case MuscleGroup::Back_Rhomboids:  return "Back_Rhomboids";
        case MuscleGroup::Legs_Quads:      return "Legs_Quads";
        case MuscleGroup::Legs_Glutes:     return "Legs_Glutes";
        case MuscleGroup::Legs_Hamstrings: return "Legs_Hamstrings";
        case MuscleGroup::Legs_Calves:     return "Legs_Calves";
        case MuscleGroup::Shoulders_Front: return "Shoulders_Front";
        case MuscleGroup::Shoulders_Side:  return "Shoulders_Side";
        case MuscleGroup::Shoulders_Rear:  return "Shoulders_Rear";
        case MuscleGroup::Arms_Biceps:     return "Arms_Biceps";
        case MuscleGroup::Arms_Triceps:    return "Arms_Triceps";
        case MuscleGroup::Arms_Forearms:   return "Arms_Forearms";
        case MuscleGroup::Core_Abs:        return "Core_Abs";
        case MuscleGroup::Core_Obliques:   return "Core_Obliques";
        case MuscleGroup::Core_LowerBack:  return "Core_LowerBack";
        case MuscleGroup::Count:           break;
    }
    return "Unknown";
}

MajorMuscleGroup toMajorGroup(MuscleGroup mg) noexcept {
    switch (mg) {
        case MuscleGroup::Chest_Upper:
        case MuscleGroup::Chest_Lower:
            return MajorMuscleGroup::Chest;

        case MuscleGroup::Back_Lats:
        case MuscleGroup::Back_Traps:
        case MuscleGroup::Back_Rhomboids:
            return MajorMuscleGroup::Back;

        case MuscleGroup::Legs_Quads:
        case MuscleGroup::Legs_Glutes:
        case MuscleGroup::Legs_Hamstrings:
        case MuscleGroup::Legs_Calves:
            return MajorMuscleGroup::Legs;

        case MuscleGroup::Shoulders_Front:
        case MuscleGroup::Shoulders_Side:
        case MuscleGroup::Shoulders_Rear:
            return MajorMuscleGroup::Shoulders;

        case MuscleGroup::Arms_Biceps:
        case MuscleGroup::Arms_Triceps:
        case MuscleGroup::Arms_Forearms:
            return MajorMuscleGroup::Arms;

        case MuscleGroup::Core_Abs:
        case MuscleGroup::Core_Obliques:
        case MuscleGroup::Core_LowerBack:
            return MajorMuscleGroup::Core;

        case MuscleGroup::Count:
            break;
    }
    return MajorMuscleGroup::Unknown;
}

std::optional<Exercise> Exercise::create(
    ExerciseId id,
    std::string name,
    std::string description,
    MuscleGroup primaryMuscle,
    std::vector<MuscleGroup> secondaryMuscles,
    ExerciseStructure structure,
    ExerciseKind kind,
    PurposeType purposeType,
    MovementPattern pattern,
    Equipment requiredEquipment,
    Equipment optionalEquipment,
    uint8_t difficulty,
    double met,
    int estimatedSetDurationSec,
    std::string instructions,
    std::string videoUrl,
    bool isPowerliftingLift,
    bool axialLoading,
    bool spinalFlexion,
    bool overheadMotion,
    bool deepKneeFlexion,
    bool isUnilateral,
    std::optional<ExerciseId> regressionId,
    std::optional<ExerciseId> progressionId) {
    ExerciseData data;
    data.id = id;
    data.name = std::move(name);
    data.description = std::move(description);
    data.primaryMuscle = primaryMuscle;
    data.secondaryMuscles = std::move(secondaryMuscles);
    data.structure = structure;
    data.kind = kind;
    data.purposeType = purposeType;
    data.pattern = pattern;
    data.requiredEquipment = requiredEquipment;
    data.optionalEquipment = optionalEquipment;
    data.difficulty = difficulty;
    data.met = met;
    data.estimatedSetDurationSec = estimatedSetDurationSec;
    data.instructions = std::move(instructions);
    data.videoUrl = std::move(videoUrl);
    data.isPowerliftingLift = isPowerliftingLift;
    data.axialLoading = axialLoading;
    data.spinalFlexion = spinalFlexion;
    data.overheadMotion = overheadMotion;
    data.deepKneeFlexion = deepKneeFlexion;
    data.isUnilateral = isUnilateral;
    data.regressionId = regressionId;
    data.progressionId = progressionId;

    if (!detail::validateExerciseData(data).empty()) {
        return std::nullopt;
    }

    return Exercise(data);
}

Exercise Exercise::createFromDb(const ExerciseData& data) {
    if (data.id < 0) {
        throw std::invalid_argument("Exercise::createFromDb: id must be non-negative");
    }

    return Exercise(data);
}

Exercise::Exercise(const ExerciseData& data)
    : id_(data.id),
      name_(data.name),
      description_(data.description),
      primaryMuscle_(data.primaryMuscle),
      secondaryMuscles_(data.secondaryMuscles),
      structure_(data.structure),
      kind_(data.kind),
      purposeType_(data.purposeType),
      pattern_(data.pattern),
      requiredEquipment_(data.requiredEquipment),
      optionalEquipment_(data.optionalEquipment),
      difficulty_(data.difficulty),
      met_(data.met),
      estimatedSetDurationSec_(data.estimatedSetDurationSec),
      instructions_(data.instructions),
      videoUrl_(data.videoUrl),
      isPowerliftingLift_(data.isPowerliftingLift),
      axialLoading_(data.axialLoading),
      spinalFlexion_(data.spinalFlexion),
      overheadMotion_(data.overheadMotion),
      deepKneeFlexion_(data.deepKneeFlexion),
      isUnilateral_(data.isUnilateral),
      regressionId_(data.regressionId),
      progressionId_(data.progressionId) {}

bool Exercise::isCompatibleWith(const UserConstraints& constraints) const noexcept {
    if (!hasEquipment(constraints.availableEquipment, requiredEquipment_)) {
        return false;
    }

    if (constraints.shouldAvoidAxialLoading() && axialLoading_) {
        return false;
    }

    if (constraints.shouldAvoidSpinalFlexion() && spinalFlexion_) {
        return false;
    }

    if (constraints.shouldAvoidOverheadMotion() && overheadMotion_) {
        return false;
    }

    if (constraints.shouldAvoidDeepKneeFlexion() && deepKneeFlexion_) {
        return false;
    }

    return true;
}

int Exercise::unilateralPenalty(const UserConstraints& constraints) const noexcept {
    if (!constraints.shouldPreferUnilateral()) {
        return 0;
    }

    return isUnilateral_ ? 0 : kNonPreferredUnilateralPenalty;
}

} // namespace workout::core
