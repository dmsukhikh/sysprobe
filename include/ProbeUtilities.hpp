#ifndef __PROBE_UTILITIES
#define __PROBE_UTILITIES

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <chrono>
#include <vector>
#include <optional>

namespace info
{
/* Информация хранится в структурах, чтобы мы могли удобно записывать информацию
 * в разные форматы файлов: json, csv, xml, etc.
 */
struct OSInfo
{
    std::string name;
    std::string hostname;
    std::string kernel;
    uint16_t arch; // Разрядность ОС
};

struct UserInfo
{
    /* Мы предполагаем, что используемый компилятор будет поддерживать стандарт
     * не меньше 11 версии, поэтому для хранения значений времени мы используем
     * библиотеку std::chrono. Она в стандарте, так что она обязана быть
     * переносимой.
     */

    std::string name;
    std::chrono::time_point<std::chrono::system_clock>
        lastLog; // Kогда пользователь последний раз залогинился
    std::chrono::duration<float>
        uptime; // Время активности пользователя (в секундах)
};

struct DiscPartitionInfo
{
    std::string name;
    std::string
        mountPoint; // Если мы используем c++17, можем вместо std::string
                    // использовать std::filesystem::... или подключить boost
    std::string filesystem;
    uint64_t capacity;
    uint64_t freeSpace;
};

struct PeripheryInfo
{
    std::string name;
    std::string type; // input, volume, network, etc.
};

struct NetworkInterfaceInfo
{
    std::string name;
    std::optional<std::array<uint8_t, 6>> mac{std::nullopt};   
    std::optional<std::array<uint8_t, 4>> ipv4{std::nullopt};
    std::optional<std::array<uint8_t, 16>> ipv6{std::nullopt};  
    uint8_t ipv4_mask; // хранится в сжатом виде: число единичек
    uint8_t ipv6_mask;
    /* В стандартой библиотеке C++ нет библиотеки для работы с сетью, так что
     * нет подходящего встроенного типа для хранения стат о сетевых адресах. Тем
     * не менее, существует boost::asio, но подключать ее к нашему проекту чисто
     * для хранения адресов как будто лишнее. Поэтому мы можем хранить эти
     * данные в духи языка Си, или придумать обертку над этими данными. Мы
     * склоняемся к первому варианту, потому что он проще
     */
};

struct CPUInfo
{
    std::string name;
    std::string arch;
    uint8_t cores;
    std::vector<float> load; // Загрузка каждого ядра
    uint64_t l1_cache, l2_cache, l3_cache, overall_cache; // capacity, in bytes
    // overall_cache - cache, берущийся из /proc/cpuinfo на linux
    uint64_t physid;
    float clockFreq; // in mhz
};

struct MemoryInfo
{
    uint64_t capacity;
    uint64_t freeSpace;
};

class ProbeUtilities
{
  public:


    ProbeUtilities();

    // Нам нет смысла копировать/перемещать объект интерфейса

    ProbeUtilities(const ProbeUtilities &) = delete;
    ProbeUtilities(ProbeUtilities &&) = delete;
    ProbeUtilities &operator=(const ProbeUtilities &) = delete;
    ProbeUtilities &operator=(ProbeUtilities &&) = delete;

    ~ProbeUtilities();

    OSInfo getOSInfo();

    std::vector<UserInfo> getUserInfo();

    std::vector<DiscPartitionInfo> getDiscPartitionInfo();

    std::vector<PeripheryInfo> getPeripheryInfo();

    std::vector<NetworkInterfaceInfo> getNetworkInterfaceInfo();

    MemoryInfo getMemoryInfo();

    CPUInfo getCPUInfo();

  private:
    class ProbeUtilsImpl;
    std::unique_ptr<ProbeUtilsImpl> _impl;
};
} // namespace info

#endif
