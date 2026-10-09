#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

enum class EHudComponent 
{
    ALL = 0,
    AMMO,
    ARMOUR,
    BREATH,
    CROSSHAIR,
    HEALTH,
    MONEY,
    RADAR,
    WANTED_STARS,
    WEAPON
};

struct HudPatchInfo 
{
    uintptr_t address;
    std::vector<unsigned char> original_bytes;
    std::vector<unsigned char> disabled_bytes;
    bool original_saved = false;
};

class HudManager
{
public:
    HudManager() = default;
    ~HudManager() = default;

    HudManager(const HudManager&) = delete;
    HudManager& operator=(const HudManager&) = delete;

    void Initialize();

    // These patch game code, which the game thread may be executing at the same
    // time, so requests are queued from any thread and applied by Pump() on the
    // game (render) thread. Requests from the same frame keep their order and the
    // last class-selection request wins.
    void ToggleComponent(EHudComponent component, bool toggle);
    void SetClassSelectionVisible(bool visible);

    // Applies everything queued so far. Call on the game/render thread (App::Tick).
    void Pump();

private:
    void ApplyToggle(EHudComponent component, bool toggle);
    void ApplyClassSelectionVisible(bool visible);
    void Patch(uintptr_t address, const std::vector<unsigned char>& data);

    std::mutex queue_mutex_;
    std::vector<std::pair<EHudComponent, bool>> pending_toggles_;
    std::optional<bool> pending_class_selection_;

    // Only touched by the game thread (Initialize + Pump).
    std::unordered_map<EHudComponent, HudPatchInfo> patches_;
};