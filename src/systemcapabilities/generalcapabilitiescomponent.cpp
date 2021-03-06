/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012-2016                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/systemcapabilities/generalcapabilitiescomponent.h>

#include <algorithm>
#include <sstream>

#ifdef WIN32
    #include <Windows.h>
    #include <tchar.h>
    #pragma comment(lib, "User32.lib")
    #pragma comment(lib, "Kernel32.lib")
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, DWORD);

    // These is for the 'warning C4996: 'GetVersionExA': was declared deprecated' which
    // is a known bug for VS2013
    #pragma warning(push)
    #pragma warning(disable: 4996)
    #pragma warning(suppress: 28159)

#else
    #ifdef __APPLE__
        #include <sys/sysctl.h>
    #else
        #include <sys/types.h>
        #include <sys/sysinfo.h>
        #include <cstring>
    #endif

    #include <sys/utsname.h>
#endif

namespace {
    const std::string _loggerCat = "GeneralCapabilitiesComponents";
}

namespace ghoul {
namespace systemcapabilities {
    
GeneralCapabilitiesComponent::GeneralCapabilitiesComponentError::
    GeneralCapabilitiesComponentError(std::string message)
    : RuntimeError(std::move(message), "GeneralCapabilitiesComponent")
{}
    
GeneralCapabilitiesComponent::OperatingSystemError::OperatingSystemError(std::string d,
                                                                         std::string e)
    : GeneralCapabilitiesComponentError(d + ". Error: " + e)
    , description(std::move(d))
    , errorMessage(std::move(e))
{}
    
GeneralCapabilitiesComponent::MainMemoryError::MainMemoryError(std::string message)
    : GeneralCapabilitiesComponentError(std::move(message))
{}

void GeneralCapabilitiesComponent::detectCapabilities() {
    clearCapabilities();
    detectOS();
    detectMemory();
    detectCPU();
}

void GeneralCapabilitiesComponent::clearCapabilities() {
    _operatingSystem = "";
    _installedMainMemory = 0;
    _cpu = "";
    _cores = 0;
    _cacheLineSize = 0;
    _L2Associativity = 0;
    _cacheSize = 0;
    _extensions = "";
}

void GeneralCapabilitiesComponent::detectOS() {
#ifdef WIN32
    // This code is taken from
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724429%28v=vs.85%29.aspx
    // All rights remain with their original copyright owners
    OSVERSIONINFOEX osVersionInfo;
    SYSTEM_INFO systemInfo;
    
    ZeroMemory(&systemInfo, sizeof(SYSTEM_INFO));
    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    BOOL osVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osVersionInfo);

    if (osVersionInfoEx == 0) {
        DWORD error = GetLastError();
        LPTSTR errorBuffer = nullptr;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&errorBuffer,
            0,
            NULL);
        if (errorBuffer != nullptr) {
            std::string errorMsg(errorBuffer);
            LocalFree(errorBuffer);
            throw OperatingSystemError(
                "Retrieving OS version failed. 'GetVersionEx' returned 0.",
                errorMsg
                );
        }
        throw OperatingSystemError(
            "Retrieving OS version failed. 'GetVersionEx' returned 0.",
            ""
        );
    }
    HMODULE module = GetModuleHandle(TEXT("kernel32.dll"));
    if (module == 0) {
        DWORD error = GetLastError();
        LPTSTR errorBuffer = nullptr;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&errorBuffer,
            0,
            NULL);
        if (errorBuffer != nullptr) {
            std::string errorMsg(errorBuffer);
            LocalFree(errorBuffer);
            throw OperatingSystemError(
                "Kernel32.dll handle could not be found. 'GetModuleHandle' returned 0.",
                errorMsg
                );
        }
        throw OperatingSystemError(
            "Kernel32.dll handle could not be found. 'GetModuleHandle' returned 0.",
            ""
        );
    }
    PGNSI procedureGetNativeSystemInfo = (PGNSI) GetProcAddress(
        module,
        "GetNativeSystemInfo"
    );
    if (procedureGetNativeSystemInfo != 0)
        procedureGetNativeSystemInfo(&systemInfo);
    else
        GetSystemInfo(&systemInfo);

    std::stringstream resultStream;
    if ((osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (osVersionInfo.dwMajorVersion > 4))
    {
        resultStream << "Microsoft ";
        // @TODO Add Windows 10 support ---abock
        if (osVersionInfo.dwMajorVersion == 6) {
            if (osVersionInfo.dwMinorVersion == 0) {
                if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
                    resultStream << "Windows Vista ";
                else
                    resultStream << "Windows Server 2008 ";
            }
            else if (osVersionInfo.dwMinorVersion == 1) {
                if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
                    resultStream << "Windows 7 ";
                else
                    resultStream << "Windows Server 2008 R2 ";
            }
            else if (osVersionInfo.dwMinorVersion == 2) {
                if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
                    resultStream << "Windows 8 ";
                else
                    resultStream << "Windows Server 2012 ";
            }
        }
        else if (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion == 2) {
            if (GetSystemMetrics(SM_SERVERR2))
                resultStream << "Windows Server 2003 R2";
            else if (osVersionInfo.wSuiteMask & VER_SUITE_STORAGE_SERVER)
                resultStream << "Windows Storage Server 2003";
            else if (osVersionInfo.wProductType == VER_NT_WORKSTATION &&
                systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                resultStream << "Windows XP Professional x64 Edition";
            else
                resultStream << "Windows Server 2003";
        }
        else if (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion == 1) {
            resultStream << "Windows XP ";
            if (osVersionInfo.wSuiteMask & VER_SUITE_PERSONAL)
                resultStream << "Home Edition";
            else
                resultStream << "Professional";
        }
        else if (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion == 0) {
            resultStream << "Windows 2000 ";
            if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
                resultStream << "Professional";
            else {
                if (osVersionInfo.wSuiteMask & VER_SUITE_DATACENTER)
                    resultStream << "Datacenter Server";
                else if (osVersionInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
                    resultStream << "Advanced Server";
                else
                    resultStream << "Server";
            }
        }

        if (_tcslen(osVersionInfo.szCSDVersion) > 0)
            resultStream << " " << osVersionInfo.szCSDVersion;

        resultStream << " (build " << osVersionInfo.dwBuildNumber << ")";
        
        if (osVersionInfo.dwMajorVersion >= 6) {
            if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                resultStream << ", 64-bit";
            else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
                resultStream << ", 32-bit";
        }
    }
    else
        resultStream << "OS detection failed. Version of Windows is too old.";

    _operatingSystem = resultStream.str();
#else
    utsname name;
    auto res = uname(&name);
    if (res != 0) {
        throw OperatingSystemError(
            "OS detection failed. 'uname' returned non-null value",
            std::to_string(res)
        );
    }
    std::stringstream resultStream;
    resultStream << name.sysname << " " << name.release << " "
        << name.version << " " << name.machine;

    _operatingSystem = resultStream.str();
#endif
}

void GeneralCapabilitiesComponent::detectMemory() {
#if defined(WIN32)
#ifdef GHOUL_USE_WMI
    std::string memory;
    try {
        queryWMI("Win32_ComputerSystem", "TotalPhysicalMemory", memory);
    }
    catch (const WMIError& e) {
        throw MainMemoryError(
            "Error reading physical memory from WMI. " +
            e.message + " (" + std::to_string(e.errorCode)
        );
    }
    std::stringstream convert;
    convert << memory;
    unsigned long long value;
    convert >> value;
    _installedMainMemory = static_cast<unsigned int>((value / 1024) / 1024);
#else
    ULONGLONG installedMainMemory;
    // get installed memory in kB
    BOOL success = GetPhysicallyInstalledSystemMemory(&installedMainMemory);
    if (success == TRUE)
        _installedMainMemory = static_cast<unsigned int>(installedMainMemory / 1024);
    else
        throw MainMemoryError("Error reading about of physical memory");
#endif
#elif defined(__APPLE__)
    int mib[2];
    size_t len;
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    std::vector<char> p(len);
    sysctl(mib, 2, p.data(), &len, NULL, 0);
    
    int64_t value;
    std::memcpy(&value, p.data(), sizeof(int64_t));
    _installedMainMemory = static_cast<unsigned int>((value / 1024) / 1024);
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    _installedMainMemory = static_cast<unsigned int>((memInfo.totalram / 1024) / 1024);
#endif
}

void GeneralCapabilitiesComponent::detectCPU() {
    // @TODO This function needs cleanup ---abock
#ifdef WIN32
    static const std::vector<std::string> szFeatures = {
        "fpu",
        "vme",
        "de",
        "pse",
        "tsc",
        "msr",
        "pae",
        "mce",
        "cx8",
        "apic",
        "Unknown1",
        "sep",
        "mtrr",
        "pge",
        "mca",
        "cmov",
        "pat",
        "pse36",
        "psn",
        "clflush",
        "Unknown2",
        "ds",
        "acpi", //  @TODO: "Thermal Monitor and Clock Ctrl", is this correct? ---jonasstrandstedt
        "mmx",
        "fxsr",
        "sse",
        "sse2",
        "ss",
        "ht",
        "tm",
        "Unknown4",
        "pbe"
    };

    char CPUString[0x20];
    char CPUBrandString[0x40];
    int CPUInfo[4] = { -1 };
    int nFeatureInfo = 0;
    unsigned    nIds, nExIds, i;
    bool    bSSE3NewInstructions = false;
    bool    bMONITOR_MWAIT = false;
    bool    bCPLQualifiedDebugStore = false;
    bool    bThermalMonitor2 = false;


    // __cpuid with an InfoType argument of 0 returns the number of
    // valid Ids in CPUInfo[0] and the CPU identification string in
    // the other three array elements. The CPU identification string is
    // not in linear order. The code below arranges the information 
    // in a human readable form.
    __cpuid(CPUInfo, 0);
    nIds = CPUInfo[0];
    memset(CPUString, 0, sizeof(CPUString));
    *((int*)CPUString) = CPUInfo[1];
    *((int*)(CPUString + 4)) = CPUInfo[3];
    *((int*)(CPUString + 8)) = CPUInfo[2];

    // Get the information associated with each valid Id
    for (i = 0; i <= nIds; ++i)
    {
        __cpuid(CPUInfo, i);

        // Interpret CPU feature information.
        if (i == 1)
        {
            bSSE3NewInstructions = (CPUInfo[2] & 0x1) || false;
            bMONITOR_MWAIT = (CPUInfo[2] & 0x8) || false;
            bCPLQualifiedDebugStore = (CPUInfo[2] & 0x10) || false;
            bThermalMonitor2 = (CPUInfo[2] & 0x100) || false;
            nFeatureInfo = CPUInfo[3];
        }
    }

    // Calling __cpuid with 0x80000000 as the InfoType argument
    // gets the number of valid extended IDs.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    // Get the information associated with each extended ID.
    for (i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(CPUInfo, i);

        // Interpret CPU brand string and cache information.
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000006)
        {
            _cacheLineSize = CPUInfo[2] & 0xff;
            _L2Associativity = (CPUInfo[2] >> 12) & 0xf;
            _cacheSize = (CPUInfo[2] >> 16) & 0xffff;
        }
    }

    // Get extensions list
    std::stringstream extensions;
    if (nFeatureInfo || bSSE3NewInstructions ||
        bMONITOR_MWAIT || bCPLQualifiedDebugStore ||
        bThermalMonitor2)
    {

        if (bSSE3NewInstructions)
            extensions << "sse3 ";
        if (bMONITOR_MWAIT)
            extensions << "mwait "; // @TODO:  "MONITOR/MWAIT" is this correct? ---jonasstrandstedt
        if (bCPLQualifiedDebugStore)
            extensions << "ds_cpl ";
        if (bThermalMonitor2)
            extensions << "tm2 ";
        i = 0;
        nIds = 1;
        while (i < (sizeof(szFeatures) / sizeof(const char*)))
        {
            if (nFeatureInfo & nIds)
            {
                extensions << szFeatures[i] << " ";
            }

            nIds <<= 1;
            ++i;
        }
    }

    // Set CPU name
    _cpu = CPUBrandString;

    // Set extensions and remove trailing ", "
    _extensions = extensions.str();
    if (_extensions.length() > 1)
        _extensions = _extensions.substr(0, _extensions.length()-1);

    // Get the cores
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);
    _cores = systemInfo.dwNumberOfProcessors;
#elif defined(__APPLE__)
    int mib[2];
    size_t len = 512;
    int intValue;
    
//    mib[0] = CTL_HW;
//    mib[1] = USER_CS_PATH;
//    mib[1] = HW_MODEL;
//    mib[1] = HW_MACHINE_ARCH;
    
//    sysctl(mib, 2, NULL, &len, NULL, 0);
    char p[512];
//    p = new char[len];
//    sysctl(mib, 2, p, &len, NULL, 0);
//    _cpu = p;
//    delete[] p;
    
    // CPU name
//    sysctlbyname( "machdep.cpu.brand_string", NULL, &len, NULL, 0 );
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctlbyname("machdep.cpu.brand_string", p, &len, NULL, 0 );
    _cpu = p;
//    delete[] p;
    
    // CPU features/extensions
//    sysctlbyname( "machdep.cpu.features", NULL, &len, NULL, 0 );
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctlbyname( "machdep.cpu.features", p, &len, NULL, 0 );
    _extensions = p;
//    delete[] p;
    
    // It works using reinterpret_cast<char*>(&intValue) directly
    // since the expected size is an integer. But to avoid risks
    // of memory corruption I have chosen the C-style detection
    // and conversion.
    
    // Number of cores
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;
//    sysctl(mib, 2, NULL, &len, NULL, 0);
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctl(mib, 2, p, &len, NULL, 0);
    std::memcpy(&intValue, p, sizeof(int));
    _cores = static_cast<unsigned int>(intValue);
//    delete[] p;
    
    // Cacheline size
    mib[0] = CTL_HW;
    mib[1] = HW_CACHELINE;
//    sysctl(mib, 2, NULL, &len, NULL, 0);
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctl(mib, 2, p, &len, NULL, 0);
    std::memcpy(&intValue, p, sizeof(int));
    _cacheLineSize = static_cast<unsigned int>(intValue);
//    delete[] p;
    
    // Cache size
    mib[0] = CTL_HW;
    mib[1] = HW_L2CACHESIZE;
//    sysctl(mib, 2, NULL, &len, NULL, 0);
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctl(mib, 2, p, &len, NULL, 0);
    std::memcpy(&intValue, p, sizeof(int));
    _cacheSize = static_cast<unsigned int>(intValue);
//    delete[] p;
    
    // L2 associativity
//    sysctlbyname( "machdep.cpu.cache.L2_associativity", NULL, &len, NULL, 0 );
//    p = new char[len];
    std::memset(p, 0, 512);
    sysctlbyname( "machdep.cpu.cache.L2_associativity", p, &len, NULL, 0 );
    std::memcpy(&intValue, p, sizeof(int));
    _L2Associativity = static_cast<unsigned int>(intValue);
//    delete[] p;
#else
    FILE* file;
    const unsigned int maxSize = 2048;
    char line[maxSize];

    // We must use c-style file opening because /proc is no ordinary filesystem
    file = fopen("/proc/cpuinfo", "r");
    if(file) {
        while(fgets(line, maxSize, file) != NULL){
            if (strncmp(line, "processor", 9) == 0) ++_cores;
            if (strncmp(line, "model name", 10) == 0) {
                _cpu = line;
                _cpu = _cpu.substr(18, _cpu.length()-19);
            }
            if (strncmp(line, "cache size", 10) == 0) {
                std::string tmp = line;
                tmp = tmp.substr(13, tmp.length() - 14);
                _cacheSize = static_cast<unsigned int>(strtol(tmp.c_str(), NULL, 0));

            }
            if (strncmp(line, "flags", 5) == 0) {
                _extensions = line;
                _extensions = _extensions.substr(9, _extensions.length() - 10);
            }
            memset(line, 0, maxSize);
        }
        fclose(file);
    }

    file = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
    if(file) {
        if(fgets(line, maxSize, file) != NULL){
            _cacheLineSize = static_cast<unsigned int>(strtol(line, NULL, 0));
        }
        fclose(file);
    }

    file = fopen("/sys/devices/system/cpu/cpu0/cache/index0/ways_of_associativity", "r");
    if(file) {
        if(fgets(line, maxSize, file) != NULL){
            _L2Associativity = static_cast<unsigned int>(strtol(line, NULL, 0));
        }
        fclose(file);
    }
#endif
}

struct Foo {
    std::string a;
    std::string b;
    SystemCapabilitiesComponent::Verbosity c;
};

std::vector<SystemCapabilitiesComponent::CapabilityInformation>
    GeneralCapabilitiesComponent::capabilities() const
{   
    std::vector<SystemCapabilitiesComponent::CapabilityInformation> result;

    Foo f = {"1", "2", SystemCapabilitiesComponent::Verbosity::Minimal };

    SystemCapabilitiesComponent::CapabilityInformation i = { std::string("foo"), std::string("bar"), SystemCapabilitiesComponent::Verbosity::Minimal };

    result.push_back({ "Operating System", _operatingSystem, Verbosity::Minimal });
    result.push_back({ "CPU", _cpu, Verbosity::Default });
    result.push_back({ "Cores", coresAsString(), Verbosity::Default });
    result.push_back({ "Cache line size", cacheLineSizeAsString(), Verbosity::Full });
    result.push_back({ "L2 Associativity", L2AssiciativityAsString(), Verbosity::Full });
    result.push_back({ "Cache size", cacheSizeAsString(), Verbosity::Full });
    result.push_back({ "Extensions", _extensions,Verbosity::Full });
    result.push_back({ "Main Memory", installedMainMemoryAsString(), Verbosity::Default });
    return result;
}

const std::string& GeneralCapabilitiesComponent::operatingSystem() const {
    return _operatingSystem;
}

unsigned int GeneralCapabilitiesComponent::installedMainMemory() const {
    return _installedMainMemory;
}

std::string GeneralCapabilitiesComponent::installedMainMemoryAsString() const {
    return std::to_string(_installedMainMemory) + " MB";
}

unsigned int GeneralCapabilitiesComponent::cores() const {
    return _cores;
}

unsigned int GeneralCapabilitiesComponent::cacheLineSize() const {
    return _cacheLineSize;
}

unsigned int GeneralCapabilitiesComponent::L2Associativity() const {
    return _L2Associativity;
}

unsigned int GeneralCapabilitiesComponent::cacheSize() const {
    return _cacheSize;
}

std::string GeneralCapabilitiesComponent::coresAsString() const {
    return std::to_string(_cores);
}

std::string GeneralCapabilitiesComponent::cacheLineSizeAsString() const {
    return std::to_string(_cacheLineSize);
}

std::string GeneralCapabilitiesComponent::L2AssiciativityAsString() const {
    return std::to_string(_L2Associativity);
}

std::string GeneralCapabilitiesComponent::cacheSizeAsString() const {
    return std::to_string(_cacheSize) + " KB";
}

std::string GeneralCapabilitiesComponent::extensions() const {
    return _extensions;
}

std::string GeneralCapabilitiesComponent::name() const {
    return "CPU";
}

} // namespace systemcapabilities
} // namespace ghoul

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
