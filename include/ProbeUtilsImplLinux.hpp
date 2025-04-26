#ifndef __PROBE_UTILS_IMPL_LINUX
#define __PROBE_UTILS_IMPL_LINUX
#include <ProbeUtilities.hpp>
#include <unordered_map>
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

    CPUInfo getCPUInfo();

  private:
    static const std::unordered_map<std::string, decltype(OSInfo::arch)>
        _OS_BITNESS;
    std::optional<utsname> _osinfo{std::nullopt};
};

} // namespace info

#endif
