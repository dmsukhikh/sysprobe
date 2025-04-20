#if TARGET_SYSTEM == 0
#include <ProbeUtilsImplWin.hpp>
#elif TARGET_SYSTEM == 1
#include <ProbeUtilsImplLinux.hpp>
#else
static_assert(false, "Unknown target system. See CMakeLists for info");
#endif

#include "ProbeUtilities.hpp"

info::ProbeUtilities::ProbeUtilities() : _impl(new ProbeUtilsImpl) {}

info::ProbeUtilities::~ProbeUtilities() = default;

info::OSInfo info::ProbeUtilities::getOSInfo() { return _impl->getOSInfo(); }

std::vector<info::UserInfo> info::ProbeUtilities::getUserInfo()
{
    return _impl->getUserInfo();
}

std::vector<info::DiscPartitionInfo>
info::ProbeUtilities::getDiscPartitionInfo()
{
    return _impl->getDiscPartitionInfo();
}

std::vector<info::PeripheryInfo> info::ProbeUtilities::getPeripheryInfo()
{
    return _impl->getPeripheryInfo();
}

std::vector<info::NetworkInterfaceInfo>
info::ProbeUtilities::getNetworkInterfaceInfo()
{
    return _impl->getNetworkInterfaceInfo();
}

info::CPUInfo info::ProbeUtilities::getCPUInfo() { return _impl->getCPUInfo(); }
