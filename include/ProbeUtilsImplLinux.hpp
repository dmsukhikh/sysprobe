#ifndef __PROBE_UTILS_IMPL_LINUX
#define __PROBE_UTILS_IMPL_LINUX
#include <nlohmann/json.hpp>
#include <ProbeUtilities.hpp>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <sys/utsname.h>

/*
 * Класс-реализация сканирования системы для ОС Windows
 * Одновременно должен быть включен только один файл: 
 * ProbeUtilsImplLinux или ProbeUtilsImplWin
 * */

namespace info
{
class ProbeUtilities::ProbeUtilsImpl
{
  public:
    ProbeUtilsImpl();
    ~ProbeUtilsImpl();
    OSInfo getOSInfo();

    std::vector<UserInfo> getUserInfo();

    std::vector<DiscPartitionInfo> getDiscPartitionInfo();

    std::vector<PeripheryInfo> getPeripheryInfo();

    std::vector<NetworkInterfaceInfo> getNetworkInterfaceInfo();

    MemoryInfo getMemoryInfo();

    CPUInfo getCPUInfo();

  private:
    static const std::unordered_map<std::string, decltype(OSInfo::arch)>
        _OS_BITNESS;
    static const std::unordered_set<std::string> _DESIRED_CLASSES;
    std::optional<utsname> _osinfo{std::nullopt};
    std::optional<std::vector<DiscPartitionInfo>> _cached_DPInfo{std::nullopt};

    void _getCPULoadness(CPUInfo &write);
    void _getCPUCache(CPUInfo &write);
    void _getCPUBasicInfo(CPUInfo &write);
};

} // namespace info

#endif
