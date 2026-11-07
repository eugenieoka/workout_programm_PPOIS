#pragma once

#include <string>

#include "core/Exercise.h"

namespace workout::core::detail {

/**
 * @brief Validate exercise data before constructing an Exercise.
 * @return Empty string if valid, otherwise newline-separated error messages.
 */
[[nodiscard]] std::string validateExerciseData(const ExerciseData& data);

} // namespace workout::core::detail
