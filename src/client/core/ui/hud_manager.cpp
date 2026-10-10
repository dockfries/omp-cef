#include "hud_manager.hpp"
#include <samp/addresses.hpp>

void HudManager::Initialize()
{
    patches_[EHudComponent::AMMO] = { 0x5893B0, {}, {0xC3}, false };
    patches_[EHudComponent::ARMOUR] = { 0x5890A0, {}, {0xC3}, false };
    patches_[EHudComponent::BREATH] = { 0x589190, {}, {0xC3}, false };
    patches_[EHudComponent::CROSSHAIR] = { 0x58E020, {}, {0xC3}, false };
    patches_[EHudComponent::HEALTH] = { 0x589270, {}, {0xC3}, false };
    patches_[EHudComponent::MONEY] = { 0x58F47D, {}, {0x90, 0xE9}, false };
    patches_[EHudComponent::RADAR] = { 0x58A330, {}, {0xC3}, false };
    patches_[EHudComponent::WANTED_STARS] = { 0x58D9A0, {}, {0xC3}, false };
    patches_[EHudComponent::WEAPON] = { 0x58D7D0, {}, {0xC3}, false };

    LOG_INFO("[HudManager] Initialized.");
}

void HudManager::ToggleComponent(EHudComponent component, bool toggle)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    pending_toggles_.emplace_back(component, toggle);
}

void HudManager::SetClassSelectionVisible(bool visible)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    pending_class_selection_ = visible;
}

void HudManager::Pump()
{
    std::vector<std::pair<EHudComponent, bool>> toggles;
    std::optional<bool> class_selection;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        toggles.swap(pending_toggles_);
        class_selection.swap(pending_class_selection_);
    }

    for (const auto& [component, toggle] : toggles)
    {
        ApplyToggle(component, toggle);
    }

    if (class_selection.has_value())
    {
        ApplyClassSelectionVisible(*class_selection);
    }
}

// The HUD patch addresses below are absolute addresses inside gta_sa.exe, so they are only valid
// while the executable sits at its image base, and a bad read or write is an access violation
// rather than a C++ exception (try/catch cannot catch it).
enum class PageAccess
{
    Read,
    Write
};

static bool GameImageIsAtExpectedBase()
{
    constexpr uintptr_t kExpectedImageBase = 0x400000;
    return reinterpret_cast<uintptr_t>(::GetModuleHandleA(nullptr)) == kExpectedImageBase;
}

static bool IsAccessiblePointer(const void* pointer, size_t bytes, PageAccess access)
{
    if (!pointer || bytes == 0)
        return false;

    MEMORY_BASIC_INFORMATION info{};
    if (::VirtualQuery(pointer, &info, sizeof(info)) == 0)
        return false;

    if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;

    if (access == PageAccess::Write && (info.Protect & (PAGE_READONLY | PAGE_EXECUTE_READ)) != 0)
        return false;

    const auto start = reinterpret_cast<uintptr_t>(pointer);
    const auto end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return start + bytes <= end;
}

static bool IsWritablePointer(const void* pointer, size_t bytes)
{
    return IsAccessiblePointer(pointer, bytes, PageAccess::Write);
}

void HudManager::ApplyToggle(EHudComponent component, bool toggle)
{
    if (component == EHudComponent::ALL) {
        for (const auto& kv : patches_) {
            ApplyToggle(kv.first, toggle);
        }

        return;
    }

    auto it = patches_.find(component);
    if (it == patches_.end())
        return;

    auto& patch = it->second;

    if (!GameImageIsAtExpectedBase() ||
        !IsAccessiblePointer(reinterpret_cast<const void*>(patch.address), patch.disabled_bytes.size(), PageAccess::Read))
    {
        LOG_WARN("[HudManager] Refusing to patch {:X}: unexpected game image base or unreadable target.", patch.address);
        return;
    }

    // Capture original bytes once so we can restore later
    if (!patch.original_saved) {
        patch.original_bytes.resize(patch.disabled_bytes.size());
        std::memcpy(patch.original_bytes.data(), reinterpret_cast<void*>(patch.address), patch.original_bytes.size());
        patch.original_saved = true;
    }

    if (toggle) {
        Patch(patch.address, patch.original_bytes);
    }
    else {
        Patch(patch.address, patch.disabled_bytes);
    }
}

void HudManager::ApplyClassSelectionVisible(bool visible)
{
    const auto version = SampAddresses::Instance().Version();
    const auto base = SampAddresses::Instance().Base();

    if (base == nullptr) {
        LOG_WARN("[HudManager] Cannot set class selection visibility: SA-MP base address is null.");
        return;
    }

    uintptr_t ptr_offset = 0;
    constexpr uintptr_t flag_offset = 0x13;

    switch (version)
    {
        case SampVersion::V037:    
            ptr_offset = 0x21A18C; 
            break;
        case SampVersion::V037R3:  
            ptr_offset = 0x26E974; 
            break;
        case SampVersion::V037R5:  
            ptr_offset = 0x26EC2C; 
            break;
        case SampVersion::V03DLR1: 
            ptr_offset = 0x2ACABC; 
            break;
        default:
            LOG_WARN("[HudManager] Unsupported SA-MP version for class selection visibility toggle.");
            return;
    }

    if (ptr_offset == 0) {
        LOG_WARN("[HudManager] Offset for this SA-MP version is not configured.");
        return;
    }

    try
    {
        const auto storage = reinterpret_cast<const void*>(base + ptr_offset);
        if (!IsAccessiblePointer(storage, sizeof(uintptr_t), PageAccess::Read))
        {
            LOG_WARN("[HudManager] Class selection pointer storage is not readable - visibility patch skipped.");
            return;
        }

        auto pClassSelection = *reinterpret_cast<const uintptr_t*>(storage);
        if (pClassSelection == 0)
            return;

        auto* pVisibilityFlag = reinterpret_cast<uint8_t*>(pClassSelection + flag_offset);
        if (!IsWritablePointer(pVisibilityFlag, sizeof(uint8_t)))         {             LOG_WARN("[HudManager] Class selection object is not ready ({}) - visibility patch skipped.",                 reinterpret_cast<void*>(pClassSelection));             return;         } 
        *pVisibilityFlag = visible ? 1 : 0;

        LOG_DEBUG("[HudManager] Class selection visibility set to {}.", visible ? 1 : 0);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[HudManager] Exception while toggling class selection visibility: {}", e.what());
    }
}

void HudManager::Patch(uintptr_t address, const std::vector<unsigned char>& data)
{
    if (!address || data.empty())
        return;

    if (!GameImageIsAtExpectedBase() ||
        !IsAccessiblePointer(reinterpret_cast<const void*>(address), data.size(), PageAccess::Read))
    {
        LOG_ERROR("[HudManager] Refusing to patch {:X}: unexpected game image base or unreadable target.", address);
        return;
    }

    DWORD oldProtect{};
    if (VirtualProtect(reinterpret_cast<void*>(address), data.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        std::memcpy(reinterpret_cast<void*>(address), data.data(), data.size());
        DWORD tmp{};
        VirtualProtect(reinterpret_cast<void*>(address), data.size(), oldProtect, &tmp);
    }
}
