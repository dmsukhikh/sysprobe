#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplWin.hpp>
#include <iostream> // for debugging
#include <sstream>
#include "windows.h"
#include <lm.h>
#include <string>
#include <array>
#include <memory>
#include <cstdio>  // for _popen and _pclose
#include <stdexcept>
#include <chrono>


using putils = info::ProbeUtilities;
putils::ProbeUtilsImpl::ProbeUtilsImpl() 
{
    std::cout << "compiled for windows" << std::endl;
}

putils::ProbeUtilsImpl::~ProbeUtilsImpl() {}

info::OSInfo putils::ProbeUtilsImpl::getOSInfo() { 
    OSInfo result;
    result.name = _execCommand(
        "(Get-WmiObject Win32_OperatingSystem).Caption");
    result.hostname = _execCommand(
        "(Get-WmiObject Win32_ComputerSystem).Name");
    result.kernel = _execCommand(
        "(Get-WmiObject Win32_OperatingSystem).Version");
    std::string arch_s = _execCommand(
        "(Get-WmiObject Win32_OperatingSystem).OSArchitecture");

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
    // используется single-user Windows 
    UserInfo user;
    user.name = _execCommand("$env:USERNAME");

    std::string uptimeStr = _execCommand(
        "((Get-Date) - (Get-CimInstance Win32_LogonSession | Where-Object { "
        "$_.LogonType -eq 2 } | Sort-Object StartTime | Select-Object -First "
        "1).StartTime).TotalSeconds");

    try
    {
        user.uptime = std::chrono::duration<float>(std::stof(uptimeStr));
    }
    catch (...)
    {
        user.uptime = std::chrono::seconds(0);
    }

    // Последний логин = now - uptime
    user.lastLog =
        std::chrono::system_clock::now() -
        std::chrono::duration_cast<std::chrono::seconds>(user.uptime);
    return std::vector<info::UserInfo>{user};
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

info::MemoryInfo putils::ProbeUtilsImpl::getMemoryInfo()
{
    MEMORYSTATUSEX memStatus = {};
    memStatus.dwLength = sizeof(memStatus);

    MemoryInfo memInfo{};

    memInfo.capacity = 0;
    memInfo.freeSpace = 0;
    if (GlobalMemoryStatusEx(&memStatus))
    {
        memInfo.capacity = memStatus.ullTotalPhys;
        memInfo.freeSpace = memStatus.ullAvailPhys;
    }

    return memInfo;
}

std::string putils::ProbeUtilsImpl::_execCommand(const std::string& command) const
{
    // Описание безопасности для 17 стандарта
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE hReadPipe, hWritePipe;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return "Error: Failed to create pipe";
    }

    // Настройка STARTUPINFO
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};

    std::string fullCommand = "powershell -Command \"" + command + "\"";
    char cmdLine[MAX_PATH * 4];
    strncpy(cmdLine, fullCommand.c_str(), sizeof(cmdLine) - 1);

    BOOL success =
        CreateProcessA(nullptr, cmdLine, nullptr, nullptr,
                       TRUE,             // Наследование дескрипторов
                       CREATE_NO_WINDOW, // Не показывать окно консоли
                       nullptr, nullptr,
                       &si, // STARTUPINFO
                       &pi  // PROCESS_INFORMATION
        );

    CloseHandle(hWritePipe);

    if (!success)
    {
        CloseHandle(hReadPipe);
        return "Error: Failed to create process";
    }

    // Чтение результата из pipe
    std::ostringstream output;
    std::array<char, 256> buffer;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, buffer.data(), 
           buffer.size() - 1, &bytesRead, nullptr) &&
           bytesRead != 0)
    {
        buffer[bytesRead] = '\0';
        output << buffer.data();
    }

    // Закрытие всех дескрипторов
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::string result = output.str();
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}
