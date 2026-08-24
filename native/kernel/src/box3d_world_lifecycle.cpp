#include "box3d_world_lifecycle.hpp"

#include "box3d_world_lifecycle_probe.hpp"

#include <atomic>
#include <mutex>

namespace ninho::physics::detail {
namespace {

std::mutex& lifecycle_mutex() noexcept
{
    static std::mutex value;
    return value;
}

std::atomic<std::size_t>& live_entries() noexcept
{
    static std::atomic<std::size_t> value{0U};
    return value;
}

std::atomic<std::size_t>& peak_entries() noexcept
{
    static std::atomic<std::size_t> value{0U};
    return value;
}

// Counts one entry into the guarded region and keeps the observed peak. A
// correct guard never lets two entries overlap, so the peak stays at 1; drop
// the mutex and contending threads push it higher.
class LifecycleEntry {
public:
    LifecycleEntry() noexcept
    {
        const std::size_t live = live_entries().fetch_add(1U) + 1U;
        std::size_t peak = peak_entries().load();
        while (peak < live
            && !peak_entries().compare_exchange_weak(peak, live)) {
        }
    }

    LifecycleEntry(const LifecycleEntry&) = delete;
    LifecycleEntry& operator=(const LifecycleEntry&) = delete;

    ~LifecycleEntry() { live_entries().fetch_sub(1U); }
};

}

b3WorldId create_box3d_world(const b3WorldDef& definition) noexcept
{
    const std::lock_guard<std::mutex> guard{lifecycle_mutex()};
    const LifecycleEntry entry;
    return b3CreateWorld(&definition);
}

void destroy_box3d_world(b3WorldId world) noexcept
{
    const std::lock_guard<std::mutex> guard{lifecycle_mutex()};
    const LifecycleEntry entry;
    b3DestroyWorld(world);
}

std::size_t box3d_world_lifecycle_peak_concurrency() noexcept
{
    return peak_entries().load();
}

void reset_box3d_world_lifecycle_peak_concurrency() noexcept
{
    peak_entries().store(live_entries().load());
}

}
