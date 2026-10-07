#include "samp_version_manager.hpp"
#include "system/logger.hpp"

#include <vector>

namespace
{
    struct KnownSampBuild
    {
        DWORD timestamp;
        DWORD sizeOfImage;
        SampVersion version;
    };

    constexpr KnownSampBuild kKnownSampBuilds[] = {
        { 0x5542F47A, 0x330000, SampVersion::V037 },
        { 0x5C0B4243, 0x27E000, SampVersion::V037R3 },
        { 0x6372C39E, 0x27E000, SampVersion::V037R5 },
        { 0x5A6A3130, 0x2BE000, SampVersion::V03DLR1 },
    };

    SampVersion DetectByPeHeader(HMODULE module, DWORD& timestamp, DWORD& sizeOfImage)
    {
        const auto* base = reinterpret_cast<const BYTE*>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return SampVersion::Unknown;

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return SampVersion::Unknown;

        timestamp = nt->FileHeader.TimeDateStamp;
        sizeOfImage = nt->OptionalHeader.SizeOfImage;

        for (const auto& build : kKnownSampBuilds)
        {
            if (build.timestamp == timestamp && build.sizeOfImage == sizeOfImage)
                return build.version;
        }

        return SampVersion::Unknown;
    }
}

bool SampVersionManager::Initialize()
{
    while ((_sampModule = ::GetModuleHandleA("samp.dll")) == nullptr)
        ::Sleep(100);

    if (!_sampModule)
    {
        LOG_ERROR("samp.dll module handle is null.");
        _version = SampVersion::Unknown;
        return false;
    }

    DWORD timestamp = 0;
    DWORD sizeOfImage = 0;
    _version = DetectByPeHeader(_sampModule, timestamp, sizeOfImage);
    if (_version != SampVersion::Unknown)
    {
        LOG_INFO("Detected SA-MP version : {} (PE signature)", GetVersionString());
        return true;
    }

    LOG_DEBUG("SA:MP PE signature not recognized (timestamp=0x{:08X}, size=0x{:X}), falling back to version info.", timestamp, sizeOfImage);

    wchar_t path[MAX_PATH]{};
    if (GetModuleFileNameW(_sampModule, path, MAX_PATH) == 0)
    {
        LOG_ERROR("Failed to retrieve samp.dll path.");
        _version = SampVersion::Unknown;
        return false;
    }

    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size == 0)
    {
        LOG_ERROR("Failed to retrieve file version info size.");
        _version = SampVersion::Unknown;
        return false;
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
    {
        LOG_ERROR("Failed to read file version info.");
        _version = SampVersion::Unknown;
        return false;
    }

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &len))
    {
        LOG_ERROR("Failed to query version info.");
        _version = SampVersion::Unknown;
        return false;
    }

    const int major    = (fileInfo->dwFileVersionMS >> 16) & 0xFFFF;
    const int minor    = (fileInfo->dwFileVersionMS >>  0) & 0xFFFF;
    const int build    = (fileInfo->dwFileVersionLS >> 16) & 0xFFFF;
    const int revision = (fileInfo->dwFileVersionLS >>  0) & 0xFFFF;

    LOG_DEBUG("SA:MP version detected : {}.{}.{}.{}", major, minor, build, revision);

    if      (major == 0 && minor == 3 && build == 7 && revision == 0) 
        _version = SampVersion::V037;
    else if (major == 0 && minor == 3 && build == 7 && revision == 2) 
        _version = SampVersion::V037R3;
    else if (major == 0 && minor == 3 && build == 7 && revision == 5) 
        _version = SampVersion::V037R5;
    else if (major == 0 && minor == 3 && build == 8 && revision == 0) 
        _version = SampVersion::V03DLR1;
    else                                                              
        _version = SampVersion::Unknown;

    LOG_INFO("Detected SA-MP version : {}", GetVersionString());
    return true;
}

const char* SampVersionManager::GetVersionString() const
{
    switch (_version)
    {
        case SampVersion::V037:    
            return "0.3.7-R1";
        case SampVersion::V037R3:  
            return "0.3.7-R3-1";
        case SampVersion::V037R5:  
            return "0.3.7-R5-1";
        case SampVersion::V03DLR1: 
            return "0.3.DL-R1";
        default:                   
            return "Unknown";
    }
}