#include "core/Exercise.h"

#include "EquipmentNames.h"
#include "ExerciseValidation.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace workout::core {
namespace {

using json = nlohmann::json;

const char* structureToString(ExerciseStructure structure) noexcept {
    switch (structure) {
        case ExerciseStructure::Compound:  return "Compound";
        case ExerciseStructure::Isolation: return "Isolation";
    }
    return "Unknown";
}

const char* kindToString(ExerciseKind kind) noexcept {
    switch (kind) {
        case ExerciseKind::Resistance: return "Resistance";
        case ExerciseKind::Olympic:    return "Olympic";
        case ExerciseKind::Cardio:     return "Cardio";
        case ExerciseKind::Mobility:   return "Mobility";
    }
    return "Unknown";
}

const char* purposeToString(PurposeType purpose) noexcept {
    switch (purpose) {
        case PurposeType::Main:         return "Main";
        case PurposeType::Accessory:    return "Accessory";
        case PurposeType::Corrective:   return "Corrective";
        case PurposeType::Conditioning: return "Conditioning";
        case PurposeType::Warmup:       return "Warmup";
    }
    return "Unknown";
}

const char* patternToString(MovementPattern pattern) noexcept {
    switch (pattern) {
        case MovementPattern::Squat:           return "Squat";
        case MovementPattern::Hinge:           return "Hinge";
        case MovementPattern::Lunge:           return "Lunge";
        case MovementPattern::Push_Horizontal: return "Push_Horizontal";
        case MovementPattern::Push_Vertical:   return "Push_Vertical";
        case MovementPattern::Pull_Horizontal: return "Pull_Horizontal";
        case MovementPattern::Pull_Vertical:   return "Pull_Vertical";
        case MovementPattern::Carry:           return "Carry";
        case MovementPattern::Rotation:        return "Rotation";
        case MovementPattern::Other:           return "Other";
    }
    return "Unknown";
}

MuscleGroup muscleGroupFromString(std::string_view value) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(MuscleGroup::Count); ++i) {
        const auto muscle = static_cast<MuscleGroup>(i);
        if (value == toString(muscle)) {
            return muscle;
        }
    }
    throw std::runtime_error("Unknown MuscleGroup value: " + std::string(value));
}

ExerciseStructure structureFromString(std::string_view value) {
    if (value == "Compound")  return ExerciseStructure::Compound;
    if (value == "Isolation") return ExerciseStructure::Isolation;
    throw std::runtime_error("Unknown ExerciseStructure value: " + std::string(value));
}

ExerciseKind kindFromString(std::string_view value) {
    if (value == "Resistance") return ExerciseKind::Resistance;
    if (value == "Olympic")    return ExerciseKind::Olympic;
    if (value == "Cardio")     return ExerciseKind::Cardio;
    if (value == "Mobility")   return ExerciseKind::Mobility;
    throw std::runtime_error("Unknown ExerciseKind value: " + std::string(value));
}

PurposeType purposeFromString(std::string_view value) {
    if (value == "Main")         return PurposeType::Main;
    if (value == "Accessory")    return PurposeType::Accessory;
    if (value == "Corrective")   return PurposeType::Corrective;
    if (value == "Conditioning") return PurposeType::Conditioning;
    if (value == "Warmup")       return PurposeType::Warmup;
    throw std::runtime_error("Unknown PurposeType value: " + std::string(value));
}

MovementPattern patternFromString(std::string_view value) {
    if (value == "Squat")           return MovementPattern::Squat;
    if (value == "Hinge")           return MovementPattern::Hinge;
    if (value == "Lunge")           return MovementPattern::Lunge;
    if (value == "Push_Horizontal") return MovementPattern::Push_Horizontal;
    if (value == "Push_Vertical")   return MovementPattern::Push_Vertical;
    if (value == "Pull_Horizontal") return MovementPattern::Pull_Horizontal;
    if (value == "Pull_Vertical")   return MovementPattern::Pull_Vertical;
    if (value == "Carry")           return MovementPattern::Carry;
    if (value == "Rotation")        return MovementPattern::Rotation;
    if (value == "Other")           return MovementPattern::Other;
    throw std::runtime_error("Unknown MovementPattern value: " + std::string(value));
}

template <typename T>
T valueOr(const json& object, const char* key, T fallback) {
    if (!object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<T>();
}

std::vector<MuscleGroup> muscleGroupsFromJson(const json& value) {
    if (!value.is_array()) {
        throw std::runtime_error("SecondaryMuscles must be an array of strings");
    }

    std::vector<MuscleGroup> result;
    for (const auto& item : value) {
        if (!item.is_string()) {
            throw std::runtime_error("SecondaryMuscles must be an array of strings");
        }
        result.push_back(muscleGroupFromString(item.get<std::string>()));
    }
    return result;
}

std::optional<ExerciseId> optionalIdFromJson(const json& object, const char* key) {
    if (!object.contains(key) || object[key].is_null()) {
        return std::nullopt;
    }
    return object[key].get<ExerciseId>();
}

uint8_t difficultyFromJson(const json& root) {
    const int value = valueOr<int>(root, "Difficulty", 3);
    if (value < 0 || value > 255) {
        throw std::runtime_error("Difficulty must be between 0 and 255");
    }
    return static_cast<uint8_t>(value);
}

} // namespace

std::string Exercise::toJson() const {
    json root;

    root["Id"] = id_;
    root["Name"] = name_;
    root["Description"] = description_;
    root["PrimaryMuscle"] = toString(primaryMuscle_);

    root["SecondaryMuscles"] = json::array();
    for (const MuscleGroup muscle : secondaryMuscles_) {
        root["SecondaryMuscles"].push_back(toString(muscle));
    }

    root["Structure"] = structureToString(structure_);
    root["Kind"] = kindToString(kind_);
    root["PurposeType"] = purposeToString(purposeType_);
    root["MovementPattern"] = patternToString(pattern_);
    root["RequiredEquipment"] = detail::equipmentToStrings(requiredEquipment_);
    root["OptionalEquipment"] = detail::equipmentToStrings(optionalEquipment_);
    root["Difficulty"] = difficulty_;
    root["Met"] = met_;
    root["EstimatedSetDurationSec"] = estimatedSetDurationSec_;
    root["Instructions"] = instructions_;
    root["VideoUrl"] = videoUrl_;
    root["IsPowerliftingLift"] = isPowerliftingLift_;

    root["Flags"] = {
        {"AxialLoading", axialLoading_},
        {"SpinalFlexion", spinalFlexion_},
        {"OverheadMotion", overheadMotion_},
        {"DeepKneeFlexion", deepKneeFlexion_},
        {"IsUnilateral", isUnilateral_}
    };

    root["RegressionId"] = regressionId_ ? json(*regressionId_) : json(nullptr);
    root["ProgressionId"] = progressionId_ ? json(*progressionId_) : json(nullptr);

    return root.dump(2);
}

Exercise Exercise::fromJson(const std::string& text) {
    try {
        const json root = json::parse(text);
        if (!root.is_object()) {
            throw std::runtime_error("Root must be a JSON object");
        }

        ExerciseData data;
        data.id = valueOr<ExerciseId>(root, "Id", -1);
        data.name = root.at("Name").get<std::string>();
        data.description = valueOr<std::string>(root, "Description", {});
        data.primaryMuscle =
            muscleGroupFromString(root.at("PrimaryMuscle").get<std::string>());

        if (root.contains("SecondaryMuscles") && !root["SecondaryMuscles"].is_null()) {
            data.secondaryMuscles = muscleGroupsFromJson(root["SecondaryMuscles"]);
        }

        if (root.contains("Structure") && !root["Structure"].is_null()) {
            data.structure = structureFromString(root["Structure"].get<std::string>());
        }

        if (root.contains("Kind") && !root["Kind"].is_null()) {
            data.kind = kindFromString(root["Kind"].get<std::string>());
        }

        if (root.contains("PurposeType") && !root["PurposeType"].is_null()) {
            data.purposeType = purposeFromString(root["PurposeType"].get<std::string>());
        }

        if (root.contains("MovementPattern") && !root["MovementPattern"].is_null()) {
            data.pattern =
                patternFromString(root["MovementPattern"].get<std::string>());
        }

        if (root.contains("RequiredEquipment") && !root["RequiredEquipment"].is_null()) {
            data.requiredEquipment = detail::equipmentFromJson(root["RequiredEquipment"]);
        }

        if (root.contains("OptionalEquipment") && !root["OptionalEquipment"].is_null()) {
            data.optionalEquipment = detail::equipmentFromJson(root["OptionalEquipment"]);
        }

        data.difficulty = difficultyFromJson(root);
        data.met = valueOr<double>(root, "Met", 0.0);
        data.estimatedSetDurationSec =
            valueOr<int>(root, "EstimatedSetDurationSec", 60);
        data.instructions = valueOr<std::string>(root, "Instructions", {});
        data.videoUrl = valueOr<std::string>(root, "VideoUrl", {});
        data.isPowerliftingLift = valueOr<bool>(root, "IsPowerliftingLift", false);

        if (root.contains("Flags") && !root["Flags"].is_null()) {
            const json& flags = root["Flags"];
            if (!flags.is_object()) {
                throw std::runtime_error("Flags must be a JSON object");
            }
            data.axialLoading = valueOr<bool>(flags, "AxialLoading", false);
            data.spinalFlexion = valueOr<bool>(flags, "SpinalFlexion", false);
            data.overheadMotion = valueOr<bool>(flags, "OverheadMotion", false);
            data.deepKneeFlexion = valueOr<bool>(flags, "DeepKneeFlexion", false);
            data.isUnilateral = valueOr<bool>(flags, "IsUnilateral", false);
        }

        data.regressionId = optionalIdFromJson(root, "RegressionId");
        data.progressionId = optionalIdFromJson(root, "ProgressionId");

        const std::string validationError = detail::validateExerciseData(data);
        if (!validationError.empty()) {
            throw std::runtime_error("Invalid exercise data: " + validationError);
        }

        return Exercise(data);
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("Exercise::fromJson: ") + error.what());
    }
}

} // namespace workout::core
