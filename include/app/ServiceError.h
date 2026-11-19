#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace workout::app {

/**
 * @brief Application-layer error with a stable machine-readable code.
 *
 * The console UI maps codes to Russian messages. A future iOS bridge can map
 * the same codes to NSError userInfo values without parsing the message text.
 */
class ServiceError : public std::runtime_error {
public:
    ServiceError(std::string code, std::string message)
        : std::runtime_error(std::move(message)), code_(std::move(code)) {}

    [[nodiscard]] const std::string& code() const noexcept { return code_; }

private:
    std::string code_;
};

} // namespace workout::app
