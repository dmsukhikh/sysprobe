#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplLinux.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <stdexcept>
#include <thread>
#include <utmp.h>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <arpa/inet.h>



using putils = info::ProbeUtilities;
using namespace nlohmann;
using namespace std::chrono_literals;

// Хардкод по всем возможным значениям uname -m
const std::unordered_map<std::string, decltype(info::OSInfo::arch)>
    putils::ProbeUtilsImpl::_OS_BITNESS = {
        {"alpha", 64},      {"arc", 32},       {"arm", 32},
        {"aarch64_be", 64}, {"aarch64", 64},   {"armv8b", 32},
        {"armv8l", 32},     {"blackfin", 32},  {"c6x", 32},
        {"cris", 32},       {"frv", 32},       {"h8300", 32},
        {"hexagon", 32},    {"ia64", 64},      {"m32r", 32},
        {"m68k", 32},       {"metag", 32},     {"microblaze", 32},
        {"mips", 32},       {"mips64", 64},    {"mn10300", 32},
        {"nios2", 32},      {"openrisk", 32},  {"parisk", 32},
        {"parisk64", 64},   {"ppc", 32},       {"ppc64", 64},
        {"ppcle", 32},      {"ppc64le", 64},   {"s390", 32},
        {"s390x", 64},      {"score", 32},     {"sh", 32},
        {"sh64", 64},       {"sparc", 32},     {"sparc64", 64},
        {"tile", 64},       {"unicore32", 32}, {"i386", 32},
        {"i686", 32},       {"x86_64", 64},    {"x64", 64},
        {"xtensa", 32},     {"arm64", 64},     {"amd64", 64},
        {"i486", 32},       {"i586", 32}};

const std::unordered_set<std::string> putils::ProbeUtilsImpl::_DESIRED_CLASSES =
    {"multimedia", "communication", "printer", "input", "display"};

putils::ProbeUtilsImpl::ProbeUtilsImpl() 
{
}

putils::ProbeUtilsImpl::~ProbeUtilsImpl() {}

info::OSInfo putils::ProbeUtilsImpl::getOSInfo() 
{  
    if (!_osinfo.has_value())
    {
        _osinfo.emplace();
        uname(&_osinfo.value());
    }

    // Для того, чтобы не писать везде .value();
    auto &osinfo = _osinfo.value();
    OSInfo output = {osinfo.sysname, osinfo.nodename, osinfo.release, 0}; 

    try
    {
        auto arch = _OS_BITNESS.at(osinfo.machine); 
        output.arch = arch;
    }
    catch (const std::out_of_range &)
    {
        output.arch = 0; 
    }

    return output;
}

std::vector<info::UserInfo> putils::ProbeUtilsImpl::getUserInfo()
{
    // Не кэшируется, потому что пользователи могут добавится в 
    // рантайме

    std::fstream utmpFile("/var/run/utmp",
                           std::ios_base::in | std::ios_base::binary);
    if (!utmpFile)
    {
        return {};
    }
    
    std::vector<UserInfo> output;
    for (utmp userEntryRaw;
         utmpFile.read(reinterpret_cast<char *>(&userEntryRaw), sizeof(utmp));)
    {
        UserInfo userEntry;
        if (userEntryRaw.ut_type == USER_PROCESS)
        {
            userEntry.name = userEntryRaw.ut_user;
            const std::time_t raw_last_log = userEntryRaw.ut_tv.tv_sec;
            userEntry.lastLog =
                std::chrono::system_clock::from_time_t(raw_last_log);
            userEntry.uptime =
                std::chrono::system_clock::now() - userEntry.lastLog;
            output.push_back(userEntry);
        }
    }

    return output;
}

std::vector<info::DiscPartitionInfo>
putils::ProbeUtilsImpl::getDiscPartitionInfo()
{
    if (!_cached_DPInfo.has_value())
    {
        _cached_DPInfo.emplace();

        std::system(
            "lsblk --output NAME,MOUNTPOINT,FSTYPE,SIZE,FSAVAIL,FSUSED,TYPE "
            "--json --bytes > dpinfo.json");
        std::ifstream dpinfo("dpinfo.json");
        json dpinfoParsed = json::parse(dpinfo);

        auto &output = _cached_DPInfo.value();
        // Generic lambda для удобного извлечения значения, которое
        // может быть null
        auto extract = [](const json &couple, const auto defval)
        { return !couple.is_null() ? couple.get<decltype(defval)>() : defval; };

        for (const auto &disc : dpinfoParsed["blockdevices"])
        {
            for (const auto &part : disc["children"])
            {
                DiscPartitionInfo infoToPush{
                    extract(part["name"], std::string{"null"}),
                    extract(part["mountpoint"], std::string{"null"}),
                    extract(part["fstype"], std::string{"null"}),
                    extract(part["size"], uint64_t{0}),
                    extract(part["fsavail"], uint64_t{0})};
                output.push_back(infoToPush);
            }
        }

        dpinfo.close();
        std::system("rm dpinfo.json");
    }

    return _cached_DPInfo.value();
}

std::vector<info::PeripheryInfo> putils::ProbeUtilsImpl::getPeripheryInfo()
{
    // Не кэшируется, потому что периферийные устройства могут быть подключены
    // в рантаймe

    std::system("lshw -json > perinfo.json");
    std::ifstream rawPerInfo("perinfo.json");

    // Реализуем DFS для мощного обхода json'a
    std::vector<PeripheryInfo> output;

    std::function<void(const json &)> traverse =
        [&traverse, &output](const json &entry)
    {
        auto cls = entry["class"].get<std::string>();
        if (_DESIRED_CLASSES.count(cls))
        {
            std::string name;

            if (entry.count("vendor"))
                name += entry["vendor"].get<std::string>() + " ";
            if (entry.count("product"))
                name += entry["product"].get<std::string>() + " ";
            if (entry.count("description"))
                name += "| " + entry["description"].get<std::string>() + " ";
            if (entry.count("id"))
                name += entry["id"].get<std::string>() + " ";

            name.pop_back();

            output.push_back({name, cls});
        }

        if (entry.count("children"))
        {
            for (const auto &child : entry["children"])
            {
                traverse(child);
            }
        }
    };

    traverse(json::parse(rawPerInfo));
    rawPerInfo.close();
    std::system("rm perinfo.json");

    return output;
}

std::vector<info::NetworkInterfaceInfo>
putils::ProbeUtilsImpl::getNetworkInterfaceInfo()
{
    std::system("ip -j addr show > netinfo.json");
    std::ifstream rawNInfo("netinfo.json");
        
    std::vector<NetworkInterfaceInfo> output;
    for (const auto &interface: json::parse(rawNInfo))
    {
        NetworkInterfaceInfo curIF;
        // Такое се, нужно поменять интерфейс
        curIF.ipv4 = std::nullopt;
        curIF.ipv6 = std::nullopt;
        curIF.mac = std::nullopt;


        curIF.name = interface["ifname"].get<std::string>();
        if (interface.count("address"))
        {
            std::stringstream macraw(interface["address"].get<std::string>());
            std::array<uint8_t, 6> mac;
            int i = 0;
            std::string temp;
            while (std::getline(macraw, temp, ':'))
            {
                mac[i++] = std::stoi(temp, nullptr, 16); 
            }
            curIF.mac = mac;
        }

        // Ищем ip-адреса
        if (interface.count("addr_info"))
        {
            for (const auto &ipinfo: interface["addr_info"])
            {
                if (ipinfo["family"].get<std::string>() == "inet")
                {
                    curIF.ipv4.emplace();
                    std::string pres = ipinfo["local"].get<std::string>();
                    char networkformat[4];
                    inet_pton(AF_INET, pres.c_str(), &networkformat);
                    std::copy(std::begin(networkformat),
                              std::end(networkformat),
                              curIF.ipv4.value().begin());
                    curIF.ipv4_mask = ipinfo["prefixlen"].get<uint8_t>();
                }
                else if (ipinfo["family"].get<std::string>() == "inet6")
                {
                    curIF.ipv6.emplace();
                    std::string pres = ipinfo["local"].get<std::string>();
                    char networkformat[sizeof(in6_addr)];
                    inet_pton(AF_INET6, pres.c_str(), networkformat);
                    std::copy(std::begin(networkformat),
                              std::end(networkformat),
                              curIF.ipv6.value().begin());
                    curIF.ipv6_mask = ipinfo["prefixlen"].get<uint8_t>();
                }
            }
        }
        output.push_back(curIF);
    }

    rawNInfo.close();
    std::system("rm netinfo.json");

    return output;
}

info::CPUInfo putils::ProbeUtilsImpl::getCPUInfo() 
{ 
    CPUInfo output;

    output.l1_cache = 0;
    output.l2_cache = 0;
    output.l3_cache = 0;

    getOSInfo();
    output.arch = _osinfo->machine;
    
    std::ifstream rawCPUInfo("/proc/cpuinfo");

    // hoho haha
    int i = 0;
    for (std::string line; std::getline(rawCPUInfo, line);)
    {
        if (line.find("model name") != std::string::npos)
        {
            output.name = std::string(line.begin() + line.find(":") + 2, line.end());
            ++i;
        }
        else if (line.find("cpu cores") != std::string::npos)
        {
            output.cores = std::stoi(
                std::string(line.begin() + line.find(":") + 2, line.end()));
            ++i;
        }
        if (i == 2) break;
    }
    
    // Получаем емкость кэшей
    std::system("lscpu -C --json --bytes > cpuinfo.json");
    std::ifstream cacheInfoRaw("cpuinfo.json");
    auto cacheInfo = json::parse(cacheInfoRaw);

    for (const auto &entry: cacheInfo["caches"])
    {
        auto name = entry["name"].get<std::string>();
        uint64_t capacity = std::stoll(entry["all-size"].get<std::string>());

        if (name == "L1d")
        {
            output.l1_cache = capacity;
        }
        else if (name == "L2")
        {
            output.l2_cache = capacity;
        }
        else if (name == "L3")
        {
            output.l3_cache = capacity;
        }
    }

    // Получаем загруженность процессора
    auto getTicks = []()
    {
        std::vector<std::pair<uint64_t, uint64_t>> output;
        std::system("cat /proc/stat | grep \"cpu\" > loadinfo.txt");
        std::ifstream st("loadinfo.txt");

        for (std::string line, stub; getline(st, line); )
        {
            std::stringstream parsedLine(line);
            parsedLine >> stub;
            if (stub == "cpu") continue;

            // Числа под номерами 4,5 - такты в простое
            uint64_t all = 0, useful = 0;
            for (uint64_t i = 1, cur; parsedLine; ++i)
            {
                parsedLine >> cur;
                if (!(i == 4 || i == 5)) useful += cur;
                all += cur;
            }
            output.emplace_back(useful, all);
        }
        return output;
    };

    auto f1 = getTicks();
    std::this_thread::sleep_for(1s);
    auto f2 = getTicks();

    for (std::size_t i = 0; i < f2.size(); ++i)
    {
        std::pair<uint64_t, uint64_t> diff = {f2[i].first - f1[i].first,
                                              f2[i].second - f1[i].second};
        output.load.push_back(static_cast<double>(diff.first)/diff.second);
    }

    return output;
}

