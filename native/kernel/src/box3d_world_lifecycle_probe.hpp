#pragma once

#include <cstddef>

namespace ninho::physics::detail {

// Observability for the Box3D world lifecycle guard, kept free of Box3D types
// so test targets can read it without taking a dependency on box3d headers.
// See box3d_world_lifecycle.hpp for why the guard exists.

// Highest number of threads observed inside the guarded region since the last
// reset. The guard is correct only while this stays at 1.
[[nodiscard]] std::size_t box3d_world_lifecycle_peak_concurrency() noexcept;
void reset_box3d_world_lifecycle_peak_concurrency() noexcept;

}
