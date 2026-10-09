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

// The class selection object lives inside the SA-MP client image, so the pointer read from it
// cannot be trusted: confirm the page is committed and writable before touching it. A bad write
// here is an access violation, not a C++ exception, so try/catch cannot catch it.
static bool IsWritablePointer(const void* pointer, size_t bytes)
{
    if (!pointer)
        return false;

    MEMORY_BASIC_INFORMATION info{};
    if (::VirtualQuery(pointer, &info, sizeof(info)) == 0)
        return false;

    if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;

    const auto start = reinterpret_cast<uintptr_t>(pointer);
    const auto end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return start + bytes <= end;
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
        auto pClassSelection = *reinterpret_cast<uintptr_t*>(base + ptr_offset);
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

    DWORD oldProtect{};
    if (VirtualProtect(reinterpret_cast<void*>(address), data.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        std::memcpy(reinterpret_cast<void*>(address), data.data(), data.size());
        DWORD tmp{};
        VirtualProtect(reinterpret_cast<void*>(address), data.size(), oldProtect, &tmp);
    }
}
