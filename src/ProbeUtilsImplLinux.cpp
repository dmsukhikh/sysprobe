#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplLinux.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <thread>
#include <utmp.h>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>



using putils = info::ProbeUtilities;
using namespace nlohmann;
using namespace std::chrono_literals;

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
    output.arch = sysconf(_SC_LONG_BIT);

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

        for (const auto &disc :
             dpinfoParsed.value("blockdevices", nlohmann::basic_json<>{}))
        {
            for (const auto &part :
                 disc.value("children", nlohmann::basic_json<>{}))
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
        // generic используется в lshw, когда не указано класса
        auto cls = entry.value("class", std::string("generic"));
        if (_DESIRED_CLASSES.count(cls) != 0)
        {
            std::string name;

            name += entry.value("vendor", std::string()) + " ";
            name += entry.value("product", std::string()) + " ";
            name += "| " + entry.value("description", std::string()) + " ";
            name += entry.value("id", std::string()) + " ";

            name.pop_back();

            output.push_back({name, cls});
        }

        for (const auto &child :
             entry.value("children", nlohmann::basic_json<>{}))
        {
            traverse(child);
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
        curIF.name = interface.value("ifname", std::string());

        // Если address не указан, продолжать дальше нет смысла
        if (interface.contains("address"))
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
        if (interface.contains("addr_info"))
        {
            // Если поле addr_info есть, то остальные аттрибуты также
            // указываются утилитой ip, значит, можно не оборачивать их в value

            for (const auto &ipinfo : interface["addr_info"])
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

    _getCPUBasicInfo(output); 
    _getCPUCache(output);
    _getCPULoadness(output);

    return output;
}

void putils::ProbeUtilsImpl::_getCPULoadness(CPUInfo &output) 
{
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
}

void putils::ProbeUtilsImpl::_getCPUCache(CPUInfo &output) 
{
    // Получаем емкость кэшей
    std::system("lscpu -C --json --bytes > cpuinfo.json");
    std::ifstream cacheInfoRaw("cpuinfo.json");
    auto cacheInfo = json::parse(cacheInfoRaw);

    for (const auto &entry :
         cacheInfo.value("caches", nlohmann::basic_json<>{}))
    {
        // Если поле caches есть, то остальные аттрибуты также
        // указываются утилитой lscpu, значит, можно не оборачивать их в value
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
}

void putils::ProbeUtilsImpl::_getCPUBasicInfo(CPUInfo &output) 
{
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
        else if (line.find("physical id") != std::string::npos)
        {
            output.physid = std::stoi(
                std::string(line.begin() + line.find(":") + 2, line.end()));
            ++i;
        }
        else if (line.find("cpu MHz") != std::string::npos)
        {
            output.clockFreq = std::stof(
                std::string(line.begin() + line.find(":") + 2, line.end()));
            ++i;
        }
        else if (line.find("cache size") != std::string::npos)
        {
            // Я надеюсь, что вывод /proc/cpuinfo не поменяется
            output.overall_cache = std::stoi(
                std::string(line.begin() + line.find(":") + 2, line.end() - 3));
            output.overall_cache *= 1024;
            ++i;
        }
        if (i == 5) break;
    }
}

info::MemoryInfo putils::ProbeUtilsImpl::getMemoryInfo()
{
    MemoryInfo output;
    std::ifstream cacheInfoRaw("/proc/meminfo");
    for (std::string line, it; getline(cacheInfoRaw, line);)
    {
        std::stringstream lineParsed(line);
        if (line.find("MemTotal") != std::string::npos)
        {
            // Скипаем 
            lineParsed >> it;
            lineParsed >> it;
            // Там все дается в килобайтах
            output.capacity = std::stoll(it) * 1024; 
        }
        else if (line.find("MemAvailable") != std::string::npos)
        {
            // Скипаем 
            lineParsed >> it;
            lineParsed >> it;
            output.freeSpace = std::stoll(it) * 1024; 
            // Дальше парсить смысла нет
            break;
        }
    }
    return output;
}

