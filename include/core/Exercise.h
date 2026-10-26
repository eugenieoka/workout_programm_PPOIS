#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "Types.h"

namespace workout::core {

// MuscleGroup - Major + Minor (18 groups)

enum class MuscleGroup : uint8_t {
    Chest_Upper   = 0,
    Chest_Lower   = 1,
    Back_Lats     = 2,
    Back_Traps    = 3,
    Back_Rhomboids= 4,
    Legs_Quads    = 5,
    Legs_Glutes   = 6,
    Legs_Hamstrings = 7,
    Legs_Calves   = 8,
    Shoulders_Front = 9,
    Shoulders_Side  = 10,
    Shoulders_Rear  = 11,
    Arms_Biceps   = 12,
    Arms_Triceps  = 13,
    Arms_Forearms = 14,
    Core_Abs      = 15,
    Core_Obliques = 16,
    Core_LowerBack= 17,
    Count         = 18  
};

enum class MajorMuscleGroup : uint8_t {
    Chest = 0, Back = 1, Legs = 2, Shoulders = 3, Arms = 4, Core = 5, Unknown = 6   
};

[[nodiscard]] const char* toString(MuscleGroup mg) noexcept;
[[nodiscard]] MajorMuscleGroup toMajorGroup(MuscleGroup mg) noexcept;

// ExerciseStructure - multi-joint vs single-joint

enum class ExerciseStructure : uint8_t {
    Compound  = 0,
    Isolation = 1
};

// ExerciseKind — modality

enum class ExerciseKind : uint8_t {
    Resistance = 0,
    Olympic    = 1,
    Cardio     = 2,
    Mobility   = 3
};

// PurposeType - role in program

enum class PurposeType : uint8_t {
    Main        = 0,
    Accessory   = 1,
    Corrective  = 2,
    Conditioning= 3,
    Warmup      = 4
};

// MovementPattern

enum class MovementPattern : uint8_t {
    Squat           = 0,
    Hinge           = 1,
    Lunge           = 2,
    Push_Horizontal = 3,
    Push_Vertical   = 4,
    Pull_Horizontal = 5,
    Pull_Vertical   = 6,
    Carry           = 7,
    Rotation        = 8,
    Other           = 9
};

struct UserConstraints;

// ExerciseData - for createFromDb

struct ExerciseData {
    ExerciseId id = -1;
    std::string name;
    std::string description;
    MuscleGroup primaryMuscle = MuscleGroup::Chest_Upper;
    std::vector<MuscleGroup> secondaryMuscles;
    ExerciseStructure structure = ExerciseStructure::Compound;
    ExerciseKind kind = ExerciseKind::Resistance;
    PurposeType purposeType = PurposeType::Main;
    MovementPattern pattern = MovementPattern::Other;
    Equipment requiredEquipment = Equipment::None;
    Equipment optionalEquipment = Equipment::None;
    uint8_t difficulty = 3;
    double met = 0.0;
    int estimatedSetDurationSec = 60;
    std::string instructions;
    std::string videoUrl;
    bool isPowerliftingLift = false;
    bool axialLoading = false;
    bool spinalFlexion = false;
    bool overheadMotion = false;
    bool deepKneeFlexion = false;
    bool isUnilateral = false;
    std::optional<ExerciseId> regressionId;
    std::optional<ExerciseId> progressionId;
};

// Exercise

class Exercise {
public:
    // Factories 
    static std::optional<Exercise> create(
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
        Equipment optionalEquipment = Equipment::None,
        uint8_t difficulty = 3,
        double met = 0.0,
        int estimatedSetDurationSec = 60,
        std::string instructions = {},
        std::string videoUrl = {},
        bool isPowerliftingLift = false,
        bool axialLoading = false,
        bool spinalFlexion = false,
        bool overheadMotion = false,
        bool deepKneeFlexion = false,
        bool isUnilateral = false,
        std::optional<ExerciseId> regressionId = std::nullopt,
        std::optional<ExerciseId> progressionId = std::nullopt
    );

    static Exercise createFromDb(const ExerciseData& data);

    Exercise() = default;

    // Getters
    [[nodiscard]] ExerciseId                      getId() const noexcept { return id_; }
    [[nodiscard]] const std::string&              getName() const noexcept { return name_; }
    [[nodiscard]] const std::string&              getDescription() const noexcept { return description_; }
    [[nodiscard]] MuscleGroup                     getPrimaryMuscle() const noexcept { return primaryMuscle_; }
    [[nodiscard]] const std::vector<MuscleGroup>& getSecondaryMuscles() const noexcept { return secondaryMuscles_; }
    [[nodiscard]] ExerciseStructure               getStructure() const noexcept { return structure_; }
    [[nodiscard]] ExerciseKind                    getKind() const noexcept { return kind_; }
    [[nodiscard]] PurposeType                     getPurposeType() const noexcept { return purposeType_; }
    [[nodiscard]] MovementPattern                 getMovementPattern() const noexcept { return pattern_; }
    [[nodiscard]] Equipment                       getRequiredEquipment() const noexcept { return requiredEquipment_; }
    [[nodiscard]] Equipment                       getOptionalEquipment() const noexcept { return optionalEquipment_; }
    [[nodiscard]] uint8_t                         getDifficulty() const noexcept { return difficulty_; }
    [[nodiscard]] double                          getMET() const noexcept { return met_; }
    [[nodiscard]] int                             getEstimatedSetDurationSec() const noexcept { return estimatedSetDurationSec_; }
    [[nodiscard]] const std::string&              getInstructions() const noexcept { return instructions_; }
    [[nodiscard]] const std::string&              getVideoUrl() const noexcept { return videoUrl_; }
    [[nodiscard]] bool                            isPowerliftingLift() const noexcept { return isPowerliftingLift_; }

    // Constraint flags
    [[nodiscard]] bool hasAxialLoading()    const noexcept { return axialLoading_; }
    [[nodiscard]] bool hasSpinalFlexion()   const noexcept { return spinalFlexion_; }
    [[nodiscard]] bool hasOverheadMotion()  const noexcept { return overheadMotion_; }
    [[nodiscard]] bool hasDeepKneeFlexion() const noexcept { return deepKneeFlexion_; }
    [[nodiscard]] bool isUnilateral()       const noexcept { return isUnilateral_; }

    // Substitution chain
    [[nodiscard]] std::optional<ExerciseId> getRegressionId() const noexcept { return regressionId_; }
    [[nodiscard]] std::optional<ExerciseId> getProgressionId() const noexcept { return progressionId_; }

    // Compatibility
    [[nodiscard]] bool isCompatibleWith(const UserConstraints& constraints) const noexcept;

    // Ranking penalty for preferUnilateral
    /**
     * @brief Penalty score for ranking. 0 = preferred, >0 = less preferred.
     * Used by PlanGenerator when userPreferUnilateral is set.
     */
    [[nodiscard]] int unilateralPenalty(const UserConstraints& c) const noexcept;

    // Serialization 
    [[nodiscard]] std::string toJson() const;
    static Exercise fromJson(const std::string& json);

private:
    ExerciseId id_ = -1;
    std::string name_;
    std::string description_;
    MuscleGroup primaryMuscle_ = MuscleGroup::Chest_Upper;
    std::vector<MuscleGroup> secondaryMuscles_;
    ExerciseStructure structure_ = ExerciseStructure::Compound;
    ExerciseKind kind_ = ExerciseKind::Resistance;
    PurposeType purposeType_ = PurposeType::Main;
    MovementPattern pattern_ = MovementPattern::Other;
    Equipment requiredEquipment_ = Equipment::None;
    Equipment optionalEquipment_ = Equipment::None;
    uint8_t difficulty_ = 3;
    double met_ = 0.0;
    int estimatedSetDurationSec_ = 60;
    std::string instructions_;
    std::string videoUrl_;
    bool isPowerliftingLift_ = false;

    bool axialLoading_ = false;
    bool spinalFlexion_ = false;
    bool overheadMotion_ = false;
    bool deepKneeFlexion_ = false;
    bool isUnilateral_ = false;

    std::optional<ExerciseId> regressionId_;
    std::optional<ExerciseId> progressionId_;

    explicit Exercise(const ExerciseData& data);
};

}