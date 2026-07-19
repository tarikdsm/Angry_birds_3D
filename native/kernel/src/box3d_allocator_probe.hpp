#pragma once

#include <cstdint>

namespace ninho::physics::detail {

// Process-global Box3D allocation total, not ownership for one PhysicsWorld.
// Baseline/delta consumers must prevent overlapping worlds or concurrent
// scenario runs inside the measured process.
[[nodiscard]] std::int64_t box3d_allocator_byte_count() noexcept;

}
