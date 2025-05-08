#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplWin.hpp>
#include <iostream> // for debugging
#include "windows.h"
#include <lm.h>
#include <string>
#include <array>
#include <memory>
#include <cstdio>  // for _popen and _pclose
#include <stdexcept>


using putils = info::ProbeUtilities;
putils::ProbeUtilsImpl::ProbeUtilsImpl() 
{
    std::cout << "compiled for windows" << std::endl;
}

putils::ProbeUtilsImpl::~ProbeUtilsImpl() {}

info::OSInfo putils::ProbeUtilsImpl::getOSInfo() { 
    OSInfo result;
    result.name = _execCommand(
        "powershell -Command \"(Get-CimInstance Win32_OperatingSystem).Caption\"");
    result.hostname = _execCommand(
        "powershell -Command \"(Get-CimInstance Win32_ComputerSystem).Name\"");
    result.kernel = _execCommand(
        "powershell -Command \"(Get-CimInstance Win32_OperatingSystem).Version\"");
    std::string arch_s = _execCommand(
        "powershell -Command \"(Get-CimInstance Win32_OperatingSystem).OSArchitecture\"");

    uint16_t arch = 0;
    if (arch_s.find("64") != std::string::npos)
    {
        arch = 64;
    } 
    else if (arch_s.find("32") != std::string::npos)
    {
        arch = 32;
    }
    result.arch = arch;
    return result;
}

std::vector<info::UserInfo> putils::ProbeUtilsImpl::getUserInfo()
{
    return std::vector<info::UserInfo>(0);
}

std::vector<info::DiscPartitionInfo>
putils::ProbeUtilsImpl::getDiscPartitionInfo()
{
    std::vector<DiscPartitionInfo> partitions;
    const size_t bufferSize = 256;
    std::array<char, bufferSize> driveBuffer{};

    DWORD result = GetLogicalDriveStringsA(static_cast<DWORD>(driveBuffer.size()), driveBuffer.data());
    if (result == 0 || result > driveBuffer.size()) {
        return partitions;
    }

    for (auto it = driveBuffer.cbegin(); it < driveBuffer.cend() && *it != '\0'; )
    {
        std::string drive(it);
        DiscPartitionInfo disk_info;

        disk_info.mountPoint = drive;
        disk_info.name = drive;

        // Get file system
        std::array<char, MAX_PATH> fileSystemName;
        GetVolumeInformationA(
            drive.c_str(),
            nullptr, 
            0, 
            nullptr, 
            nullptr, 
            nullptr,
            fileSystemName.data(), 
            sizeof fileSystemName
        );
        disk_info.filesystem = fileSystemName.data();

        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExA(
            drive.c_str(), 
            &freeBytesAvailable, 
            &totalBytes,
            &totalFreeBytes)) {
            disk_info.capacity = totalBytes.QuadPart;
            disk_info.freeSpace = totalFreeBytes.QuadPart;
        }

        partitions.push_back(disk_info);
        it += drive.size() + 1;
    }

    return partitions;
}

std::vector<info::PeripheryInfo> putils::ProbeUtilsImpl::getPeripheryInfo()
{
    return std::vector<info::PeripheryInfo>(0);
}

std::vector<info::NetworkInterfaceInfo>
putils::ProbeUtilsImpl::getNetworkInterfaceInfo()
{
    return std::vector<info::NetworkInterfaceInfo>(0);
}

info::CPUInfo putils::ProbeUtilsImpl::getCPUInfo() { return {}; }

std::string putils::ProbeUtilsImpl::_execCommand(const char* cmd) const
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&fclose)> pipe(fopen(cmd, "r"), fclose);
    // std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);

    if (pipe == nullptr) {
        result =  "Not found";
    }
    try
    {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
        {
            result += buffer.data();
        }
    }
    catch (const std::exception& e) {
        result =  "Not found";
    }
    
    return result;
}
