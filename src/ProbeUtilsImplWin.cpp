#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplWin.hpp>
#include <iostream> // for debugging
#include <sstream>
#include "windows.h"
#include <lm.h>
#include <string>
#include <array>
#include <memory>
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <regex>


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
    // ������������ single-user Windows 
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

    // ��������� ����� = now - uptime
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
    std::vector<info::PeripheryInfo> result;
       
    //������ ��� � ���
    std::regex nameRegex(R"(NAME:(.*))");
    std::regex typeRegex(R"(TYPE:(.*))");

    std::string psCommand = "$peripherals = Get-WmiObject Win32_PnPEntity; "
                            "foreach ($device in $peripherals) { "
                            "Write-Output ('NAME:' + $device.Name); "
                            "Write-Output ('TYPE:' + $device.PNPDeviceID); "
                            "}";
    std::string output = _execCommand(psCommand);

    std::string name, type;
    std::istringstream iss(output);
    std::string line;

    while (std::getline(iss, line))
    {
        line.erase(line.find_last_not_of(" \r\n") + 1);
        std::smatch match;

        if (std::regex_match(line, match, nameRegex))
        {
            name = match[1];
        }
        else if (std::regex_match(line, match, typeRegex))
        {
            type = match[1];
            if (!name.empty())
            {
                result.push_back({name, type});
                name.clear();
                type.clear();
            }
        }
    }
    return result;
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
std::string putils::ProbeUtilsImpl::_execCommand(const std::string &command) const
{
    // ������� pipe (�������� �������� ������������� ��� �++17)
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return "Error: Failed to create pipe";
    }

    // ��������� STARTUPINFO
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};

    std::string fullCommand = "powershell -Command \"" + command + "\"";
    std::vector<char> cmdLine(fullCommand.cbegin(), fullCommand.cend());
    cmdLine.push_back('\0');

    BOOL success =
        CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr,
                       TRUE,             // ������������ ������������
                       CREATE_NO_WINDOW, // �� ���������� ���� �������
                       nullptr, nullptr,
                       &si, // STARTUPINFO
                       &pi  // PROCESS_INFORMATION
        );

    // ��������� ���������� ������ � �������� (��, ����� ���� ������)
    CloseHandle(hWritePipe);

    if (!success)
    {
        // ��������� ���������� ������ � ��������
        CloseHandle(hReadPipe);
        return "Error: Failed to create process";
    }

    // ������ ���������� �� pipe
    std::ostringstream output;
    std::array<char, 512> buffer;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, buffer.data(), buffer.size() - 1, &bytesRead,
                    nullptr) &&
           bytesRead != 0)
    {
        buffer[bytesRead] = '\0';
        output << buffer.data();
    }

    // �������� ���� ������������
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::string result = output.str();
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}
