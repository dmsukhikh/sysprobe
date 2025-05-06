#include "ProbeUtilities.hpp"
#include <ctime>
#include <chrono>
#include <iostream>
#include <sstream>


/*
 *
 * Пример того, как будет выводиться информация библиотекой
 *
 */

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
    auto result = probe.getUserInfo();
    for (const auto& d: result) {
        std::cout << "Name: " << d.name << std::endl;
        std::time_t epoch_time =
            std::chrono::system_clock::to_time_t(d.lastLog);
        std::cout << "Last log time: " << std::ctime(&epoch_time);
        std::cout
            << "Uptime: "
            << std::chrono::duration_cast<std::chrono::hours>(d.uptime).count()
            << std::endl;
        std::cout << std::endl;
    }
}

void __test_CPUInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestCPUInfo------\n";
    auto info = probe.getCPUInfo();
    std::cout << "Name: " << info.name << std::endl;
    std::cout << "Architecture: " << info.arch << std::endl;
    std::cout << "Number of cores: " << (int)info.cores << std::endl;
    std::cout << "L1 cache: " << info.l1_cache / 1024.f << " kb" << std::endl;
    std::cout << "L2 cache: " << info.l2_cache / 1024.f << " kb" << std::endl;
    std::cout << "L3 cache: " << info.l3_cache / 1024.f << " kb" << std::endl;
    std::cout << "Overall cache: " << info.overall_cache / 1024.f << " kb"
              << std::endl;
    std::cout << "Clock frequency: " << info.clockFreq << " mhz" << std::endl;
    std::cout << "physid: " << info.physid << std::endl;
    std::cout << std::endl;

    for (std::size_t i = 0; i < info.load.size(); ++i)
    {
        std::cout << "Loadness of core number " << i << ": "
                  << info.load[i] * 100 << "%" << std::endl;
    }
}

void __test_NetworkInterfaceInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestNetworkInterfaceInfo------\n";
    auto users = probe.getNetworkInterfaceInfo();

    // Печатает содержимое буфера без последнего символа и
    // очищает буфер
    auto printSSBuffer = [](std::stringstream &s)
    {
        std::string output = s.str();
        output.pop_back();
        std::cout << output;
        s.str("");
    };

    for (auto &part: users)
    {
        std::cout << "Name: " << part.name << std::endl;
        std::stringstream fs;
        if (part.mac)
        {
            std::cout << "MAC: ";
            for (auto i: part.mac.value())
            {
                fs << std::hex << (int)i << ":";
            }

            printSSBuffer(fs);
            fs << std::dec;
            std::cout << std::endl;
        }

        if (part.ipv4)
        {
            std::cout << "IPv4: ";
            for (auto i : part.ipv4.value())
            {
                fs << (int)i << ".";
            }
            printSSBuffer(fs);
            std::cout << "/" << (int)part.ipv4_mask << std::endl;
        }

        if (part.ipv6)
        {
            std::cout << "IPv6: ";
            for (auto i : part.ipv6.value())
            {
                fs << std::hex << (int)i << ":";
            }
            printSSBuffer(fs);
            fs << std::dec;
            std::cout << "/" << (int)part.ipv6_mask << std::endl;
        }
        std::cout << std::endl;
    }
}

void __test_PeripheryInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestPeripheryInfo------\n";
    auto info = probe.getPeripheryInfo();
    for (const auto &i: info)
    {
        std::cout << "Name: " << i.name << std::endl;
        std::cout << "Type: " << i.type << std::endl;
        std::cout << std::endl;
    }
}

void __test_MemoryInfo(info::ProbeUtilities& probe)
{
    std::cout << "\n------TestMemoryInfo------\n";
    auto info = probe.getMemoryInfo();
    std::cout << "Capacity: " << info.capacity / 1024.f / 1024.f << "mB"
              << std::endl;

    std::cout << "Free space: " << info.freeSpace / 1024.f / 1024.f << "mB"
              << std::endl;
}

int main() {
    info::ProbeUtilities probe;
    __test_OSInfo(probe);
    __test_UserInfo(probe);
    __test_DiscPartitionInfo(probe);
    __test_NetworkInterfaceInfo(probe);
    __test_PeripheryInfo(probe);
    __test_CPUInfo(probe);
    __test_MemoryInfo(probe);
    return 0;
}
