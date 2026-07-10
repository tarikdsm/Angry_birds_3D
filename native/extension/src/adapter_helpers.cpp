#include <ninho/extension/adapter_helpers.hpp>

#include <cmath>
#include <limits>

namespace ninho::extension::detail {

std::optional<float> checked_finite_float(double value) noexcept
{
    constexpr double float_limit = std::numeric_limits<float>::max();
    if (!std::isfinite(value) || value > float_limit || value < -float_limit) {
        return std::nullopt;
    }
    const float narrowed = static_cast<float>(value);
    return std::isfinite(narrowed) ? std::optional<float>{narrowed} : std::nullopt;
}

std::optional<float> checked_positive_float(double value) noexcept
{
    const std::optional<float> narrowed = checked_finite_float(value);
    return narrowed && *narrowed > 0.0F ? narrowed : std::nullopt;
}

}
