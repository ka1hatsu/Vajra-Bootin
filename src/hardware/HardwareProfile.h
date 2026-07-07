#pragma once

#include <cstdint>
#include <string>

namespace vajra::hardware {

struct HardwareProfile {
    std::string cpu_name{"Unknown"};
    std::string architecture{"Unknown"};
    std::string gpu_name{"Unknown"};
    std::string windows_version{"Unknown"};
    std::string firmware_mode{"Unknown"};
    std::uint32_t logical_processors{0};
    std::uint64_t memory_bytes{0};
    std::uint64_t gpu_memory_bytes{0};
    std::uint64_t system_drive_free_bytes{0};
};

} // namespace vajra::hardware
