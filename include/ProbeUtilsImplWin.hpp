#ifndef __PROBE_UTILS_IMPL_WIN
#define __PROBE_UTILS_IMPL_WIN
#include <ProbeUtilities.hpp>

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

    MemoryInfo getMemoryInfo();
  private:
    std::string _execCommand(const std::string &command) const;
};

} // namespace info

#endif
