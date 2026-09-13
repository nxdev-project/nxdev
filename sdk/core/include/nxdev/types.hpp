#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace nxdev {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

struct Version {
    u32 major{0};
    u32 minor{1};
    u32 patch{0};
    std::string_view prerelease{"beta.1"};
};

constexpr Version CURRENT_VERSION{0, 1, 0, "beta.1"};

} // namespace nxdev
