#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace workout::core {

/**
 * @brief Result of input validation — lists all errors, not just the first one.
 */
struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;      // Human-readable messages
    std::vector<std::string> errorCodes;  // Machine-readable codes

    void addError(std::string_view code, std::string_view msg) {
        isValid = false;
        errorCodes.emplace_back(code);
        errors.emplace_back(msg);
    }

    [[nodiscard]] std::string toString() const;
};

} // namespace workout::core
