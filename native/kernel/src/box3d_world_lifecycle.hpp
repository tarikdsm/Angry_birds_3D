#pragma once

#include <box3d/box3d.h>

namespace ninho::physics::detail {

// b3CreateWorld and b3DestroyWorld mutate the process-global b3_worlds table:
// creation scans it for a slot with inUse == false and only then writes
// inUse = true, which is a torn check-then-act between threads. The same calls
// also do a non-atomic read-modify-write on b3_maxWorldCount and lazily run
// b3InitializeContactRegisters behind a non-atomic flag. Box3D documents this
// in docs/foundation.md, "Multithreading Multiple Worlds": simulating separate
// worlds on separate threads is supported, but the application must guard
// creation and destruction with a mutex.
//
// Stepping a world stays unguarded. Each PhysicsWorld owns its own b3World and
// runs with workerCount 1, so there is no shared task system to serialize.
//
// Observing the guard is box3d_world_lifecycle_probe.hpp, which stays free of
// Box3D types so test targets can read it without the box3d include path.
[[nodiscard]] b3WorldId create_box3d_world(const b3WorldDef& definition) noexcept;
void destroy_box3d_world(b3WorldId world) noexcept;

}
