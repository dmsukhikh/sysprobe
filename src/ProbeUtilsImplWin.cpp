#include "windows.h"
#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplWin.hpp>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <iostream> // for debugging
#include <lm.h>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

using putils = info::ProbeUtilities;
putils::ProbeUtilsImpl::ProbeUtilsImpl()
{
    std::cout << "compiled for windows" << std::endl;
}

putils::ProbeUtilsImpl::~ProbeUtilsImpl() {}

info::OSInfo putils::ProbeUtilsImpl::getOSInfo()
{
    OSInfo result;
    std::string psCommand = "$os = Get-WmiObject Win32_OperatingSystem;"
                            "$os.Caption;"
                            "$string = $os.Name;"
                            "$index = $string.IndexOf('|');"
                            "$string.Substring($index + 1);"
                            "$os.Version;"
                            "$os.OSArchitecture;";
    std::istringstream WMI(_execCommand(psCommand));
    std::getline(WMI, result.name);
    std::getline(WMI, result.hostname);
    std::getline(WMI, result.kernel);
    std::string arch_s;
    std::getline(WMI, arch_s);

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

    std::string psCommand = _execCommand(
        "((Get-Date) - (Get-CimInstance Win32_LogonSession | Where-Object { "
        "$_.LogonType -eq 2 } | Sort-Object StartTime | Select-Object -First "
        "1).StartTime).TotalSeconds");

    try
    {
        user.uptime = std::chrono::duration<float>(std::stof(psCommand));
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

    DWORD result = GetLogicalDriveStringsA(
        static_cast<DWORD>(driveBuffer.size()), driveBuffer.data());
    if (result == 0 || result > driveBuffer.size())
    {
        return partitions;
    }

    for (auto it = driveBuffer.cbegin();
         it < driveBuffer.cend() && *it != '\0';)
    {
        std::string drive(it);
        DiscPartitionInfo disk_info;

        disk_info.mountPoint = drive;
        disk_info.name = drive;

        // Get file system
        std::array<char, MAX_PATH> fileSystemName;
        GetVolumeInformationA(drive.c_str(), nullptr, 0, nullptr, nullptr,
                              nullptr, fileSystemName.data(),
                              sizeof fileSystemName);
        disk_info.filesystem = fileSystemName.data();

        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExA(drive.c_str(), &freeBytesAvailable, &totalBytes,
                                &totalFreeBytes))
        {
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

    // Парсим тип и имя
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
            size_t pos = type.find('\\');
            if (pos != std::string::npos && std::isalpha(type[0]))
            {
                type = type.substr(0, pos);
            }

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
    std::vector<info::NetworkInterfaceInfo> result;
    // Берем сетевые адаптеры, для которых включена IP-конфигурация.
    std::string psCommand =
        "$netAdap = Get-WmiObject Win32_NetworkAdapterConfiguration | "
        "Where-Object{$_.IPEnabled}; "
        "foreach ($obj in $netAdap){ "
        "$obj.Description; "
        "$obj.MACAddress; "
        "$obj.IPAddress; "
        "$obj.IPSubnet; "
        "}";
    std::string line;
    std::istringstream iss{_execCommand(psCommand)};
    int k = 0;
    info::NetworkInterfaceInfo netInfo;

    auto countOnes = [](const std::array<uint8_t, 4> &arr)
    {
        uint32_t num = (static_cast<uint32_t>(arr[0]) << 24) |
                       (static_cast<uint32_t>(arr[1]) << 16) |
                       (static_cast<uint32_t>(arr[2]) << 8) |
                       (static_cast<uint32_t>(arr[3]));

        std::bitset<32> bits(num);
        return bits.count();
    };

    while (std::getline(iss, line))
    {
        line.erase(line.find_last_not_of(" \r\n") + 1);
        if (k == 0)
            netInfo.name = line;
        else if (k == 1)
            netInfo.mac = _splitLine<6>(line, ':', 16);
        else if (k == 2)
            netInfo.ipv4 = _splitLine<4>(line, '.', 10);
        else if (k == 3)
            netInfo.ipv6 = _splitIPv6(line);
        else if (k == 4)
            netInfo.ipv4_mask = countOnes(_splitLine<4>(line, '.', 10));
        else
        {
            netInfo.ipv6_mask = static_cast<uint8_t>(std::stoi(line));
            k = -1;
            result.push_back(netInfo);
            netInfo = info::NetworkInterfaceInfo{};
        }
        k++;
    }

    return result;
}

info::CPUInfo putils::ProbeUtilsImpl::getCPUInfo()
{
    std::unordered_map<int, std::string> archs = {
        {0, "x86"}, {1, "MIPS"},    {2, "Alpha"}, {3, "PowerPC"},
        {4, "ARM"}, {5, "Itanium"}, {6, "ia64"},  {9, "x64"},
    };

    info::CPUInfo info{};
    std::string psCommand = "$cpu = Get-WmiObject Win32_Processor;"
                            "foreach ($processor in $cpu) {"
                            "$processor.Name;"
                            "$processor.Architecture;"
                            "$processor.NumberOfCores;"
                            "$processor.L1CacheSize;"
                            "$processor.L2CacheSize;"
                            "$processor.L3CacheSize;"
                            "$processor.ProcessorId;"
                            "$processor.CurrentClockSpeed;"
                            "}";
    std::istringstream WMI(_execCommand(psCommand));
    std::string line;

    std::getline(WMI, info.name);

    std::getline(WMI, line);
    int arch = std::stoi(line);
    if (archs.count(arch))
    {
        info.arch = archs[arch];
    }
    else
    {
        info.arch = "Undefine";
    }

    std::getline(WMI, line);
    info.cores = static_cast<uint8_t>(std::stoi(line));

    info.l1_cache = 0; // Wmi пометил L1CacheSize как not implemented
    std::getline(WMI, line);
    info.l2_cache = static_cast<uint32_t>(std::stoi(line));

    std::getline(WMI, line);
    info.l3_cache = static_cast<uint32_t>(std::stoi(line));

    info.overall_cache = info.l1_cache + info.l2_cache + info.l3_cache;
    // ProcessorId - hex строка
    std::getline(WMI, line);
    info.physid = static_cast<uint64_t>(std::stoull(line, nullptr, 16));

    std::getline(WMI, line);
    info.clockFreq = std::stof(line);

    return info;
}

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
std::string putils::ProbeUtilsImpl::_execCommand(const std::string &command)
{
    // Создаем pipe (включены настрйки безопастности для с++17)
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return "Error: Failed to create pipe";
    }

    // Настройка STARTUPINFO
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
                       TRUE, // Наследование дескрипторов
                       CREATE_NO_WINDOW, // Не показывать окно консоли
                       nullptr, nullptr,
                       &si, // STARTUPINFO
                       &pi  // PROCESS_INFORMATION
        );

    // Закрываем дескриптор записи у родителя (хз, может течь походу)
    CloseHandle(hWritePipe);

    if (!success)
    {
        // Закрываем дескриптор чтения у родителя
        CloseHandle(hReadPipe);
        return "Error: Failed to create process";
    }

    // Чтение результата из pipe
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

    // Закрытие всех дескрипторов
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::string result = output.str();
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

template <size_t N>
std::array<uint8_t, N>
putils::ProbeUtilsImpl::_splitLine(const std::string &mac, char delimiter,
                                   int base)
{
    std::istringstream iss(mac);
    std::array<uint8_t, N> tokens{};
    std::string token;
    size_t i = 0;

    while (std::getline(iss, token, delimiter) && i < N)
    {
        try
        {
            tokens[i] = static_cast<uint8_t>(std::stoi(token, nullptr, base));
        }
        catch (const std::invalid_argument &e)
        {
            tokens[i] = 0;
        }
        i++;
    }

    return tokens;
}

std::array<uint8_t, 16>
putils::ProbeUtilsImpl::_splitIPv6(const std::string &ipv6)
{
    std::array<uint8_t, 16> result{};
    std::vector<std::string> segments;

    size_t zeros = ipv6.find("::");
    if (zeros != std::string::npos)
    {
        std::string left = ipv6.substr(0, zeros);
        std::string right = ipv6.substr(zeros + 2);

        std::istringstream lss(left), rss(right);
        std::string token;

        while (std::getline(lss, token, ':'))
        {
            if (!token.empty())
                segments.push_back(token);
        }

        std::vector<std::string> right_segments;
        while (std::getline(rss, token, ':'))
        {
            if (!token.empty())
                right_segments.push_back(token);
        }

        size_t fill_zeros = 8 - (segments.size() + right_segments.size());
        segments.resize(segments.size() + fill_zeros, "0");
        segments.insert(segments.end(), right_segments.begin(),
                        right_segments.end());
    }
    else
    {
        std::istringstream iss(ipv6);
        std::string token;
        while (std::getline(iss, token, ':'))
        {
            segments.push_back(token.empty() ? "0" : token);
        }
    }

    // Разбиваем каждое 16-битное слово на 2 байта (big-endian)
    size_t idx = 0;
    for (const auto &seg : segments)
    {
        if (idx + 1 >= result.size())
            break;
        uint16_t val = static_cast<uint16_t>(std::stoi(seg, nullptr, 16));
        result[idx++] = static_cast<uint8_t>((val >> 8) & 0xFF); // старший байт
        result[idx++] = static_cast<uint8_t>(val & 0xFF); // младший байт
    }

    return result;
}
