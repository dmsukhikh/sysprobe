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
    // Вызов Windows PowerShell для WMI commands
    std::string _execCommand(const std::string &command);
    // Сплит строки mac-адреса, ipv4, ipv4_mask
    template <size_t N>
    std::array<uint8_t, N> _splitLine(const std::string &mac, char delimiter,
                                      int base);
    // Вместо inet_pton
    std::array<uint8_t, 16> _splitIPv6(const std::string &ipv6);
};

} // namespace info

#endif
