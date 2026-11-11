#pragma once

#include "core/Exercise.h"

#include <optional>
#include <string>
#include <vector>

namespace workout::test {

inline core::ExerciseData validExerciseData(core::ExerciseId id = -1) {
    core::ExerciseData data;
    data.id = id;
    data.name = "Barbell Bench Press";
    data.description = "Horizontal press";
    data.primaryMuscle = core::MuscleGroup::Chest_Upper;
    data.secondaryMuscles = {
        core::MuscleGroup::Arms_Triceps,
        core::MuscleGroup::Shoulders_Front
    };
    data.structure = core::ExerciseStructure::Compound;
    data.kind = core::ExerciseKind::Resistance;
    data.purposeType = core::PurposeType::Main;
    data.pattern = core::MovementPattern::Push_Horizontal;
    data.requiredEquipment = core::Equipment::Barbell | core::Equipment::Bench;
    data.optionalEquipment = core::Equipment::None;
    data.difficulty = 3;
    data.met = 6.0;
    data.estimatedSetDurationSec = 90;
    data.instructions = "Lie on the bench and press.";
    data.videoUrl = "https://example.com/bench";
    data.isPowerliftingLift = true;
    return data;
}

inline std::optional<core::Exercise> createExercise(const core::ExerciseData& data) {
    return core::Exercise::create(
        data.id,
        data.name,
        data.description,
        data.primaryMuscle,
        data.secondaryMuscles,
        data.structure,
        data.kind,
        data.purposeType,
        data.pattern,
        data.requiredEquipment,
        data.optionalEquipment,
        data.difficulty,
        data.met,
        data.estimatedSetDurationSec,
        data.instructions,
        data.videoUrl,
        data.isPowerliftingLift,
        data.axialLoading,
        data.spinalFlexion,
        data.overheadMotion,
        data.deepKneeFlexion,
        data.isUnilateral,
        data.regressionId,
        data.progressionId);
}

inline std::optional<core::Exercise> makeValidExercise(core::ExerciseId id = -1) {
    return createExercise(validExerciseData(id));
}

} // namespace workout::test
