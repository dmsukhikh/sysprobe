#include <ProbeUtilities.hpp>
#include <ProbeUtilsImplLinux.hpp>
#include <iostream> // for debugging

using putils = info::ProbeUtilities;

putils::ProbeUtilsImpl::ProbeUtilsImpl()
{
    std::cout << "compiled for linux" << std::endl;
}

putils::ProbeUtilsImpl::~ProbeUtilsImpl() {}

info::OSInfo putils::ProbeUtilsImpl::getOSInfo() { return {}; }

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
