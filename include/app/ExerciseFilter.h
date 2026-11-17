#pragma once

#include <optional>
#include <string>

#include "core/Exercise.h"

namespace workout::app {

/**
 * @brief In-memory filter for the exercise catalog.
 *
 * All fields are optional. Empty nameContains and empty optionals match every
 * exercise. Equipment matches when the exercise requires the selected bit
 * (for example, Barbell matches Bench Press, which requires Barbell | Bench).
 */
struct ExerciseFilter {
    std::string nameContains;
    std::optional<core::MuscleGroup> primaryMuscle;
    std::optional<core::Equipment> equipment;
    std::optional<core::ExerciseKind> kind;
    std::optional<core::ExerciseStructure> structure;
    std::optional<int> maxDifficulty;

    [[nodiscard]] bool matches(const core::Exercise& exercise) const;
};

} // namespace workout::app
