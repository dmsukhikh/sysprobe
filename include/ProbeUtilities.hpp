#ifndef __PROBE_UTILITIES
#define __PROBE_UTILITIES

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace info
{
/**
 * @brief Структура, описывающая операционную систему
 */
struct OSInfo
{
    std::string name;     ///< Название операционной системы
    std::string hostname; ///< Название машины-хоста
    std::string kernel;   ///< Название ядра операционной системы
    uint16_t arch;        ///< Разрядность операционной системы
};

/**
 * @brief Структура, содержащая информацию о пользователе
 */
struct UserInfo
{
    std::string name; ///< Имя пользователя
    /**
     * @brief Когда пользователь вошел в систему
     *
     * @details Момент времени, когда пользователь вошел в систему, в секундах
     * от начала эпохи
     */
    std::chrono::time_point<std::chrono::system_clock> lastLog;
    std::chrono::duration<float>
        uptime; ///< Время активности пользователя, в секундах
};

/**
 * @brief Структура, содержащая информацию о разделе жесткого диска
 */
struct DiscPartitionInfo
{
    std::string name;       ///< Название раздела
    std::string mountPoint; ///< Точка монтирования раздела в системе
    std::string filesystem; ///< Тип файловой системы раздела

    uint64_t capacity;  ///< Емкость раздела, в байтах
    uint64_t freeSpace; ///< Емкость раздела, в байтах
};

/**
 * @brief Структура, содержащая информацию о периферийных устройствах
 */
struct PeripheryInfo
{
    std::string name; ///< Название устройства
    /**
     * @brief Тип устройства
     *
     * @note Тип устройства зависит от системы: типы на Windows не совпадают с
     * типами на Linux
     */
    std::string type;
};

/**
 * @brief Структура, содержащая информацию о сетевом интерфейсе
 */
struct NetworkInterfaceInfo
{
    std::string name; ///< Название интерфейса

    /**
     * @brief MAC-адрес устройства
     *
     * @details MAC-адрес, записанный в двоичном виде. При попытке доступа к
     * адресу необходимо проверять его наличие методами, определенными для
     * std::optional
     */
    std::optional<std::array<uint8_t, 6>> mac{std::nullopt};
    /**
     * @brief IPv4-адрес устройства
     *
     * @details IPv4-адрес, записанный в двоичном виде. При попытке доступа к
     * адресу необходимо проверять его наличие методами, определенными для
     * std::optional
     */
    std::optional<std::array<uint8_t, 4>> ipv4{std::nullopt};
    /**
     * @brief IPv6-адрес устройства
     *
     * @details IPv6-адрес, записанный в двоичном виде. При попытке доступа к
     * адресу необходимо проверять его наличие методами, определенными для
     * std::optional
     */
    std::optional<std::array<uint8_t, 16>> ipv6{std::nullopt};
    /**
     * @brief Маска IPv4-адреса
     *
     * @details Маска IPv4-адреса. Записана в сжатом виде: значение выражается в
     * виде количества подряд идущих единиц с начала маски.
     *
     * @note Если для сетевого интерфейса не существует IPv4-адреса, значение
     * поля неопределено
     */
    uint8_t ipv4_mask;
    /**
     * @brief Маска IPv6-адреса
     *
     * @details Маска IPv6-адреса. Записана в сжатом виде: значение выражается в
     * виде количества подряд идущих единиц с начала маски.
     *
     * @note Если для сетевого интерфейса не существует IPv6-адреса, значение
     * поля неопределено
     */
    uint8_t ipv6_mask;
};

/**
 * @brief Структура, описывающая процессор компьютерной системы
 */
struct CPUInfo
{
    std::string name;        ///< Название процессора
    std::string arch;        ///< Разрядность процессора
    uint8_t cores;           ///< Количество ядер
    std::vector<float> load; ///< Загрузка каждого ядра, в процентах
    uint64_t l1_cache,       ///< Емкость L1-кэша, в байтах
        l2_cache,            ///< Емкость L2-кэша, в байтах
        l3_cache,            ///< Емкость L3-кэша, в байтах
        overall_cache;       ///< Общая емкость кэша, в байтах
    uint64_t physid; ///< Physical ID процессора
    float clockFreq; ///< Текущая рабочая частота процессора, в мегагерцах
};

/**
 * @brief Структура, описывающая оперативную память
 */
struct MemoryInfo
{
    uint64_t capacity;  ///< Общая емкость ОЗУ, в байтах
    uint64_t freeSpace; ///< Доступное пространство ОЗУ, в байтах
};

/**
 * @brief Класс, предоставляющий интерфейс для сбора информации
 *
 * @details Данный класс предоставляет методы, возвращающие информацию о
 * состоянии компьютерной системы
 */
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

    /**
     * @brief Получение информации об операционной системе
     *
     * @return Заполненная структура OSInfo, содержащая информацию о целевой
     * системе
     */
    OSInfo getOSInfo();

    /**
     * @brief Получение информации о пользователях
     *
     * @details Метод собирает информацию о всех активных пользователях
     *
     * @return Массив структур UserInfo. Каждая структура описывает отдельного
     * пользователя
     */
    std::vector<UserInfo> getUserInfo();

    /**
     * @brief Получение информации о всех разделах жестких дисков
     *
     * @return Массив структур DiscPartitionInfo. Каждая структура описывает
     * отдельный раздел
     */
    std::vector<DiscPartitionInfo> getDiscPartitionInfo();

    /**
     * @brief Получение информации о всех периферийных устройствах
     *
     * @note Некоторые устройства не видны без прав суперпользователя при
     * использовании функции на Linux. Если вам необходимо получить список всех
     * устройств, вызывайте программу, использующую getPeripheryInfo(), с
     * правами суперпользователя.
     *
     *
     * @return Массив структур PeripheryInfo. Каждая структура описывает
     * отдельное периферийное устройство
     */
    std::vector<PeripheryInfo> getPeripheryInfo();

    /**
     * @brief Получение информации о всех сетевых устройствах
     *
     * @return Массив структур NetworkInterfaceInfo. Каждая структура описывает
     * отдельный сетевой интерфейс
     */
    std::vector<NetworkInterfaceInfo> getNetworkInterfaceInfo();

    /**
     * @brief Получение информации об оперативной памяти
     *
     * @return Заполненная структура MemoryInfo, содержащая информацию об
     * оперативной памяти системы
     */
    MemoryInfo getMemoryInfo();

    /**
     * @brief Получение информации о процессоре системы
     *
     * @note Предполагается, что система, на которой исполняется библиотека,
     * имеет один процессор. Поведение на мультипроцессорных системах не
     * определено.
     *
     * @return Заполненная структура CPUInfo, содержащая информацию об
     * центральном процессоре системы
     */
    CPUInfo getCPUInfo();

    MemoryInfo getMemoryInfo();

  private:
    class ProbeUtilsImpl;
    std::unique_ptr<ProbeUtilsImpl> _impl;
};
} // namespace info

#endif
