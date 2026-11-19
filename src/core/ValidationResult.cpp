#include "core/ValidationResult.h"

#include <cstddef>
#include <sstream>
#include <string>

namespace workout::core {

std::string ValidationResult::toString() const {
    if (isValid) {
        return {};
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i != 0) {
            out << '\n';
        }
        if (i < errorCodes.size()) {
            out << errorCodes[i] << ": ";
        }
        out << errors[i];
    }
    return out.str();
}

} // namespace workout::core
