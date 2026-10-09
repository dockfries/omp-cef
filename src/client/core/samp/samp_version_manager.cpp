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
        if (module == nullptr)
            return SampVersion::Unknown;

        const auto* base = reinterpret_cast<const BYTE*>(module);

        // The DOS/NT headers live in the first page of a real image. e_lfanew is a
        // raw LONG from the file, so validate it before dereferencing anything:
        // a truncated, hand-mapped or corrupt module would otherwise fault here.
        constexpr SIZE_T MaxHeaderOffset = 0x1000;

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return SampVersion::Unknown;

        const LONG e_lfanew = dos->e_lfanew;
        if (e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
            static_cast<SIZE_T>(e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > MaxHeaderOffset)
        {
            LOG_DEBUG("SA:MP PE header offset is out of range (e_lfanew=0x{:X}).", static_cast<unsigned>(e_lfanew));
            return SampVersion::Unknown;
        }

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return SampVersion::Unknown;

        if (nt->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER32) ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            LOG_DEBUG("SA:MP PE optional header is not a 32-bit PE header.");
            return SampVersion::Unknown;
        }

        const DWORD imageSize = nt->OptionalHeader.SizeOfImage;
        if (imageSize == 0)
        {
            // A loaded image always has a non-zero size, so zero means this is not
            // a real mapped image header.
            LOG_DEBUG("SA:MP PE SizeOfImage is zero.");
            return SampVersion::Unknown;
        }

        timestamp = nt->FileHeader.TimeDateStamp;
        sizeOfImage = imageSize;

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
    // samp.dll is loaded by the SA-MP client and the ASI can be injected before
    // it appears, so keep waiting - but never silently: without a log line this
    // state is indistinguishable from the plugin not being installed at all.
    // The ASI also loads in single player, where samp.dll never appears, so the
    // logging backs off after the first minute instead of repeating forever.
    constexpr int kPollIntervalMs = 100;
    constexpr long long kFastLogPolls = 50;   // every 5 s for the first minute
    constexpr long long kSlowLogPolls = 600;  // every 60 s afterwards
    constexpr long long kFastLogUntil = 600;

    // 64-bit counter: this loop has no upper bound, so an int would overflow
    // (signed overflow is UB) if the wait ever ran for years.
    long long polls = 0;

    while ((_sampModule = ::GetModuleHandleA("samp.dll")) == nullptr)
    {
        const long long logEveryNPolls = (polls < kFastLogUntil) ? kFastLogPolls : kSlowLogPolls;

        if (polls % logEveryNPolls == 0)
        {
            LOG_ERROR("Waiting for samp.dll ({} s elapsed). If this never completes, the client module is missing or has been renamed.",
                (polls * kPollIntervalMs) / 1000);
        }

        ::Sleep(kPollIntervalMs);
        ++polls;
    }

    if (!_sampModule)
    {
        LOG_ERROR("samp.dll is unavailable.");
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
    const DWORD pathLength = GetModuleFileNameW(_sampModule, path, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH)
    {
        // Truncation returns MAX_PATH, not 0, and the zero-initialised buffer
        // would otherwise hide it.
        LOG_ERROR("Failed to retrieve the samp.dll path (unavailable or truncated).");
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

    // The resource is the fallback path, so at least make sure it is a version
    // block before trusting the numbers read out of it.
    if (len < sizeof(VS_FIXEDFILEINFO) || fileInfo->dwSignature != 0xFEEF04BD)
    {
        LOG_ERROR("samp.dll version resource is missing or malformed.");
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