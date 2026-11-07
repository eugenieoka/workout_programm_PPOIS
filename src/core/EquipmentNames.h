#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/Types.h"

namespace workout::core::detail {

struct NamedEquipment {
    Equipment value;
    const char* name;
};

inline constexpr std::array<NamedEquipment, 11> kEquipmentNames{{
    {Equipment::Barbell, "Barbell"},
    {Equipment::Dumbbells, "Dumbbells"},
    {Equipment::PowerRack, "PowerRack"},
    {Equipment::CableMachine, "CableMachine"},
    {Equipment::Machines, "Machines"},
    {Equipment::PullUpBar, "PullUpBar"},
    {Equipment::DipStation, "DipStation"},
    {Equipment::Bands, "Bands"},
    {Equipment::Kettlebells, "Kettlebells"},
    {Equipment::CardioMachines, "CardioMachines"},
    {Equipment::Bench, "Bench"}
}};

inline Equipment equipmentFromString(std::string_view value) {
    if (value == "None") {
        return Equipment::None;
    }
    for (const auto& entry : kEquipmentNames) {
        if (value == entry.name) {
            return entry.value;
        }
    }
    throw std::runtime_error("Unknown Equipment value: " + std::string(value));
}

inline std::vector<std::string> equipmentToStrings(Equipment mask) {
    std::vector<std::string> result;
    for (const auto& entry : kEquipmentNames) {
        if (hasEquipment(mask, entry.value)) {
            result.emplace_back(entry.name);
        }
    }
    return result;
}

inline Equipment equipmentFromJson(const nlohmann::json& value) {
    if (!value.is_array()) {
        throw std::runtime_error("Equipment must be an array of strings");
    }

    Equipment mask = Equipment::None;
    for (const auto& item : value) {
        if (!item.is_string()) {
            throw std::runtime_error("Equipment must be an array of strings");
        }
        mask = mask | equipmentFromString(item.get<std::string>());
    }
    return mask;
}

} // namespace workout::core::detail
