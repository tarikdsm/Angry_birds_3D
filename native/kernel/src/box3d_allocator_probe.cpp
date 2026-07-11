#include "box3d_allocator_probe.hpp"

#include <box3d/base.h>

namespace ninho::physics::detail {

std::int64_t box3d_allocator_byte_count() noexcept
{
    return static_cast<std::int64_t>(b3GetByteCount());
}

}
