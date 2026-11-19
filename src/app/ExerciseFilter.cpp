#include "app/ExerciseFilter.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace workout::app {
namespace {

bool containsCaseInsensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }

    const auto equalIgnoreCase = [](char lhs, char rhs) {
        return std::tolower(static_cast<unsigned char>(lhs)) ==
               std::tolower(static_cast<unsigned char>(rhs));
    };

    return std::search(haystack.begin(), haystack.end(),
                       needle.begin(), needle.end(), equalIgnoreCase) != haystack.end();
}

} // namespace

bool ExerciseFilter::matches(const core::Exercise& exercise) const {
    if (!containsCaseInsensitive(exercise.getName(), nameContains)) {
        return false;
    }

    if (primaryMuscle && exercise.getPrimaryMuscle() != *primaryMuscle) {
        return false;
    }

    if (equipment) {
        if (*equipment == core::Equipment::None) {
            if (exercise.getRequiredEquipment() != core::Equipment::None) {
                return false;
            }
        } else if (!core::hasEquipment(exercise.getRequiredEquipment(), *equipment)) {
            return false;
        }
    }

    if (kind && exercise.getKind() != *kind) {
        return false;
    }

    if (structure && exercise.getStructure() != *structure) {
        return false;
    }

    if (maxDifficulty && exercise.getDifficulty() > *maxDifficulty) {
        return false;
    }

    return true;
}

} // namespace workout::app
