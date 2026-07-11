#pragma once

#include <compare>
#include <cstdint>

namespace ninho::simulation {

namespace detail {

template <typename Tag, typename Representation>
class StrongId {
public:
    using representation_type = Representation;

    constexpr StrongId() noexcept = default;
    constexpr explicit StrongId(Representation value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]] constexpr Representation value() const noexcept
    {
        return value_;
    }

    constexpr auto operator<=>(const StrongId&) const noexcept = default;

private:
    Representation value_{};
};

struct EntityIdTag;
struct PartIdTag;
struct MaterialIdTag;
struct JointIdTag;
struct EventIdTag;
struct TickIndexTag;

}

using EntityId = detail::StrongId<detail::EntityIdTag, std::uint32_t>;
using PartId = detail::StrongId<detail::PartIdTag, std::uint32_t>;
using MaterialId = detail::StrongId<detail::MaterialIdTag, std::uint32_t>;
using JointId = detail::StrongId<detail::JointIdTag, std::uint32_t>;
using EventId = detail::StrongId<detail::EventIdTag, std::uint64_t>;
using TickIndex = detail::StrongId<detail::TickIndexTag, std::uint64_t>;

}
