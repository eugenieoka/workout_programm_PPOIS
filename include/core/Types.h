#pragma once

#include <cstdint>

namespace workout::core {

using ExerciseId = int64_t;
using UserId     = int64_t;
using PlanId     = int64_t;
using SessionId  = int64_t;

// Equipment — bitmask (shared between UserConstraints & Exercise)

enum class Equipment : uint16_t {
    None            = 0,
    Barbell         = 1 << 0,
    Dumbbells       = 1 << 1,
    PowerRack       = 1 << 2,
    CableMachine    = 1 << 3,
    Machines        = 1 << 4,
    PullUpBar       = 1 << 5,
    DipStation      = 1 << 6,
    Bands           = 1 << 7,
    Kettlebells     = 1 << 8,
    CardioMachines  = 1 << 9,
    Bench           = 1 << 10   
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

} 