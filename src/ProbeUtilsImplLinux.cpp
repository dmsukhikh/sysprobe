#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplLinux.hpp>
#include <sys/utsname.h>
#include <stdexcept>


using putils = info::ProbeUtilities;

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
    return std::vector<info::UserInfo>(0);
}

std::vector<info::DiscPartitionInfo>
putils::ProbeUtilsImpl::getDiscPartitionInfo()
{
    return std::vector<info::DiscPartitionInfo>(0);
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
