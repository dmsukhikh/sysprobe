#include <ProbeUtilities.hpp>
#include <iostream>
#include <iomanip> // Для put_time


void __test_OSInfo(info::ProbeUtilities& probe) 
{
    std::cout << "\n------TestOSInfo------\n";
    info::OSInfo result = probe.getOSInfo();
    std::cout << "OS Name: " << result.name << std::endl;
    std::cout << "Version: " << result.hostname << std::endl;
    std::cout << "Kernel: " << result.kernel << std::endl;
    std::cout << "Arch: " << result.arch << std::endl;
}

void __test_DiscPartitionInfo(info::ProbeUtilities& probe) 
{
    std::cout << "\n------TestDiscPartitionInfo------\n";
    std::vector<info::DiscPartitionInfo> result = probe.getDiscPartitionInfo();
    for (const auto& d: result) {
        std::cout << "Name: " << d.name << std::endl;
        std::cout << "MountPoint: " << d.mountPoint << std::endl;
        std::cout << "Filesystem: " << d.filesystem << std::endl;
        std::cout << "Capacity: " << d.capacity << std::endl;
        std::cout << "FreeSpace: " << d.freeSpace << std::endl;
        std::cout << "------------------" << std::endl;
    }
}

void __test_UserInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestUserInfo------\n";
    auto user = probe.getUserInfo()[0]; //single-user windows
    std::time_t lastLogin = std::chrono::system_clock::to_time_t(user.lastLog);
    std::cout << "User:       " << user.name << "\n";
    std::cout << "Last login: "
              << std::put_time(std::localtime(&lastLogin), "%c") << "\n";
    std::cout << "Uptime:     " << user.uptime.count() << " seconds\n";
}

void __test_CPUInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestCPUInfo------\n";
}

void __test_NetworkInterfaceInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestNetworkInterfaceInfo------\n";
}

void __test_PeripheryInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestPeripheryInfo------\n";
}

int main() {
    info::ProbeUtilities probe;
    __test_OSInfo(probe);
    __test_UserInfo(probe);
    __test_DiscPartitionInfo(probe);
    __test_NetworkInterfaceInfo(probe);
    __test_PeripheryInfo(probe);
    __test_CPUInfo(probe);
    return 0;
}
