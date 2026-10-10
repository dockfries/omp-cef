#include "app.hpp"

#include <windows.h>
#include <algorithm>

#include "shared/events.hpp"
#include "shared/packet.hpp"
#include "shared/version.hpp"
#include "browser/manager.hpp"
#include "browser/audio.hpp"
#include "browser/focus.hpp"
#include "system/resource_manager.hpp"
#include "network/network_manager.hpp"
#include "ui/hud_manager.hpp"
#include "system/logger.hpp"
#include "samp/components/chat.hpp"
#include "samp/components/netgame.hpp"
#include "samp/common.hpp"
#include "utf8.hpp"

static bool SendEmitToBrowser(BrowserManager& browserManager, int browserId, const std::string& name, const std::vector<Argument>& args)
{
    // The handle is copied under the manager lock: this runs on the network thread, which must not
    // touch the browser containers, nor a browser that the CEF UI thread is dropping.
    CefRefPtr<CefBrowser> browser = browserManager.GetBrowserHandle(browserId);
    if (!browser)
    {
        // Either the browser does not exist yet or CEF has not finished creating it. The server is
        // expected to wait for the create result before emitting events, so this is worth knowing
        // about rather than dropping the event in silence.
        LOG_DEBUG("[CEF] Event '{}' for browser {} arrived before the browser was ready - dropped.", name, browserId);
        return false;
    }

    if (!browser->IsValid())
    {
        LOG_WARN("[CEF] Event '{}' dropped: browser {} is no longer valid (it is closing).", name, browserId);
        return false;
    }

    CefRefPtr<CefFrame> frame = browser->GetMainFrame();
    if (!frame)
    {
        LOG_WARN("[CEF] Event '{}' dropped: browser {} has no main frame.", name, browserId);
        return false;
    }

    if (!frame->IsValid())
    {
        LOG_WARN("[CEF] Event '{}' dropped: the main frame of browser {} is gone.", name, browserId);
        return false;
    }

    CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
    CefRefPtr<CefListValue> list = msg->GetArgumentList();

    list->SetString(0, EnsureUtf8ForCef(name));

    for (size_t i = 0; i < args.size(); ++i)
    {
        const auto& arg = args[i];
        const size_t idx = i + 1;

        switch (arg.type)
        {
            case ArgumentType::Integer:
                list->SetInt(idx, arg.intValue);
                break;

            case ArgumentType::Float:
                list->SetDouble(idx, arg.floatValue);
                break;

            case ArgumentType::Bool:
                list->SetBool(idx, arg.boolValue);
                break;

            case ArgumentType::String:
                list->SetString(idx, EnsureUtf8ForCef(arg.stringValue));
                break;

            default:
                list->SetNull(idx);
                break;
        }
    }

    // CefFrame::SendProcessMessage returns void in this CEF version (the documentation
    // states that delivery is not guaranteed and gives no result), so a failure cannot be
    // detected here. The validity checks above are the only signal available.
    frame->SendProcessMessage(PID_RENDERER, msg);

    return true;
}

void App::Initialize()
{
    LOG_INFO("Init omp-cef client app ... (v{}.{}.{})", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    network_.SetPacketHandler([this](const NetworkPacket& p) {
        OnPacketReceived(p);
    });

    resources_.Initialize();
    resources_.SetFailureHandler([this](const char* reason) { FailPendingCreates(reason); });
}

void App::ResetSession()
{
    // Clear pending actions
    {
        std::lock_guard<std::mutex> lock(pending_creates_mutex_);
        pending_creates_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(pending_emits_mutex_);
        pending_emits_.clear();
    }
    flushed_once_ = false;
    pending_clear_chat_.store(false, std::memory_order_release);

    // Destroy all browsers and drop the attach requests that belonged to the old session.
    browser_.ClearPendingAttaches();
    browser_.DestroyAllBrowsers();
}

bool App::ResourcesReady() const
{
    return resources_.GetState() == DownloadState::COMPLETED;
}

void App::FlushPendingIfReady()
{
    if (!ResourcesReady())
        return;

    if (!flushed_once_)
    {
        flushed_once_ = true;
        network_.SendPacket(PacketType::DownloadComplete, {});

        size_t pending_emit_count = 0;
        {
            std::lock_guard<std::mutex> emits_lock(pending_emits_mutex_);
            pending_emit_count = pending_emits_.size();
        }

        std::lock_guard<std::mutex> lock(pending_creates_mutex_);
        LOG_DEBUG("[CEF] Resources completed -> flushing pending creates={}, emits={}.", (int)pending_creates_.size(), (int)pending_emit_count);
    }

    {
        // Serialize queue draining and UI task posting with network layer updates.
        // Otherwise a layer update could overtake its browser's creation task.
        std::lock_guard<std::mutex> lock(pending_creates_mutex_);
        std::vector<PendingCreate> creates;
        creates.swap(pending_creates_);

        for (const auto& crate : creates)
        {
            if (crate.kind == PendingCreate::Kind::Overlay)
            {
                browser_.CreateBrowser(crate.id, crate.url, crate.focused, crate.controls_chat, crate.width, crate.height);
            }
            else if (crate.kind == PendingCreate::Kind::World)
            {
                browser_.CreateWorldBrowser(crate.id, crate.url, crate.textureName, crate.width, crate.height);
            }
            else // World2D
            {
                browser_.CreateWorld2DBrowser(crate.id, crate.url, crate.worldX, crate.worldY, crate.worldZ, crate.width, crate.height, crate.offsetZ, crate.pivotX, crate.pivotY);
            }
            if (crate.kind != PendingCreate::Kind::World && crate.layer.has_value())
                browser_.SetBrowserLayer(crate.id, *crate.layer);
        }
    }

    std::vector<PendingEmit> emits;
    {
        std::lock_guard<std::mutex> lock(pending_emits_mutex_);
        emits.swap(pending_emits_);
    }

    if (!emits.empty())
    {
        std::vector<PendingEmit> remaining;
        remaining.reserve(emits.size());

        for (const auto& emit : emits)
        {
            if (browser_.HasBrowser(emit.browserId))
            {
				SendEmitToBrowser(browser_, emit.browserId, emit.eventName, emit.args);  
            }
            else
            {
                remaining.push_back(emit);
            }
        }

        std::lock_guard<std::mutex> lock(pending_emits_mutex_);

        // Retries go first, then anything queued while we were sending.
        std::vector<PendingEmit> queued;
        queued.swap(pending_emits_);
        for (auto& emit : queued)
            remaining.push_back(std::move(emit));

        pending_emits_ = std::move(remaining);
    }
}

void App::Tick()
{
    const auto now = ::GetTickCount64();
    auto* netGame = GetComponent<NetGameComponent>();

    if (network_.GetState() != ConnectionState::DISCONNECTED)
    {
        bool should_reset_network_session = false;
        std::string reset_reason;

        if (!netGame || !netGame->IsConnected())
        {
            should_reset_network_session = true;
            reset_reason = "SA-MP is no longer connected";
        }
        else
        {
            const int localPlayerId = netGame->GetLocalPlayerId();
            const std::string host = netGame->GetIp();
            const int gamePort = netGame->GetPort();

            if (localPlayerId < 0 || host.empty() || gamePort <= 0)
            {
                should_reset_network_session = true;
                reset_reason = "invalid SA-MP connection metadata";
            }
            else if (connected_player_id_ >= 0 && localPlayerId != connected_player_id_)
            {
                should_reset_network_session = true;
                reset_reason = "SA-MP local player id changed";
            }
            else if (!connected_game_host_.empty() && (host != connected_game_host_ || gamePort != connected_game_port_))
            {
                should_reset_network_session = true;
                reset_reason = "SA-MP server endpoint changed";
            }
        }

        if (should_reset_network_session)
        {
            LOG_INFO("[CEF] Resetting network session: {}", reset_reason.c_str());

            network_.Disconnect();
            resources_.OnDisconnect();
            ResetSession();

            net_endpoint_ready_ = false;
            net_host_.clear();
            net_port_ = 0;
            connected_player_id_ = -1;
            connected_game_host_.clear();
            connected_game_port_ = 0;
            next_connect_attempt_ms_ = now + 500ULL;
        }
    }

    // A server that does not speak the CEF protocol disables networking for the
    // rest of the process. If the SA-MP endpoint is now a different one, the
    // player moved to another server, so clear the latch and try again.
    if (network_.IsNonCefServer() && net_endpoint_ready_ && netGame && netGame->IsConnected())
    {
        const std::string host = netGame->GetIp();
        const int gamePort = netGame->GetPort();
        const int cefPortInt = gamePort + cef_port_offset_;

        if (!host.empty() && gamePort > 0 && cefPortInt > 0 && cefPortInt <= 65535)
        {
            const unsigned short cefPort = static_cast<unsigned short>(cefPortInt);

            if (host != net_host_ || cefPort != net_port_)
            {
                LOG_INFO("[CEF] Endpoint is now {}:{} - re-enabling CEF networking.", host.c_str(), (int)cefPort);

                network_.ClearNonCefServerFlag();
                net_endpoint_ready_ = false;
                next_connect_attempt_ms_ = now;
            }
        }
    }

    if (network_.GetState() == ConnectionState::DISCONNECTED && !network_.IsNonCefServer())
    {
        const bool can_attempt_connect = (now >= next_connect_attempt_ms_);

        if (can_attempt_connect && netGame && netGame->IsConnected())
        {
            const int localPlayerId = netGame->GetLocalPlayerId();
            if (localPlayerId >= 0)
            {
                const std::string host = netGame->GetIp();
                const int gamePort = netGame->GetPort();

                if (!host.empty() && gamePort > 0)
                {
                    const int cefPortInt = gamePort + cef_port_offset_;

                    if (cefPortInt <= 0 || cefPortInt > 65535)
                    {
                        LOG_ERROR("[CEF] Invalid computed CEF port: gamePort={} offset={} => {}", gamePort, cef_port_offset_, cefPortInt);
                        next_connect_attempt_ms_ = now + 2000ULL;
                    }
                    else
                    {
                        const unsigned short cefPort = static_cast<unsigned short>(cefPortInt);

                        const bool endpoint_changed =
                            (!net_endpoint_ready_ || host != net_host_ || cefPort != net_port_);

                        if (endpoint_changed)
                        {
                            net_endpoint_ready_ = false;

                            if (!network_.Initialize(host, cefPort))
                            {
                                LOG_ERROR("[CEF] Failed to init endpoint {}:{} (game {} + {}) - will retry", host.c_str(), (int)cefPort, gamePort, cef_port_offset_);
                                next_connect_attempt_ms_ = now + 2000ULL;
                            }
                            else
                            {
                                net_endpoint_ready_ = true;
                                net_host_ = host;
                                net_port_ = cefPort;

                                LOG_INFO("[CEF] Endpoint {}:{} (derived from {}:{})", net_host_.c_str(), (int)net_port_, host.c_str(), gamePort);
                            }

                            LOG_DEBUG("[CEF] Server endpoint changed -> resetting session state.");
                            resources_.OnDisconnect();
                            ResetSession();
                        }

                        if (net_endpoint_ready_)
                        {
                            connected_player_id_ = localPlayerId;
                            connected_game_host_ = host;
                            connected_game_port_ = gamePort;
                            network_.Connect(localPlayerId);
                        }
                    }
                }
            }
        }
    }

    FlushPendingIfReady();

    // Queued HUD patches write to game code; apply them on this (game) thread.
    hud_.Pump();

    focus_.Update();
    browser_.TickGameData();
    browser_.CaptureScreen();
    browser_.RenderAll();
    resources_.Update(now);
    
    if (pending_clear_chat_.exchange(false, std::memory_order_acq_rel))
    {
        if (auto* chat = GetComponent<ChatComponent>())
            chat->Clear();
    }
}

void App::FailPendingCreates(const char* reason)
{
    std::vector<PendingCreate> creates;
    {
        std::lock_guard<std::mutex> lock(pending_creates_mutex_);
        creates.swap(pending_creates_);
    }

    for (const auto& crate : creates)
    {
        LOG_ERROR("[CEF] Browser {} will not be created: {}", crate.id, reason);
        network_.SendBrowserCreateResult(crate.id, false, static_cast<int>(BrowserCreateStatus::Error_Generic), reason);
    }
}

void App::RemovePendingCreate(int id)
{
    std::lock_guard<std::mutex> lock(pending_creates_mutex_);
    pending_creates_.erase(
        std::remove_if(pending_creates_.begin(), pending_creates_.end(),
            [id](const PendingCreate& pending_create) { 
                return pending_create.id == id; 
            }), pending_creates_.end());
}

void App::RemovePendingEmits(int browserId)
{
    std::lock_guard<std::mutex> lock(pending_emits_mutex_);
    pending_emits_.erase(
        std::remove_if(pending_emits_.begin(), pending_emits_.end(),
            [browserId](const PendingEmit& pending_emit) {
                return pending_emit.browserId == browserId;
            }), pending_emits_.end());
}

void App::QueueOrCreateOverlay(int id, const std::string& url, bool focused, bool controls_chat, float width, float height)
{
    std::lock_guard<std::mutex> lock(pending_creates_mutex_);
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::Overlay;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.focused = focused;
        pending_create.controls_chat = controls_chat;
        pending_create.width = width;
        pending_create.height = height;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateBrowser(id, url, focused, controls_chat, width, height);
}

void App::QueueOrCreateWorld(int id, const std::string& url, const std::string& textureName, float width, float height)
{
    std::lock_guard<std::mutex> lock(pending_creates_mutex_);
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::World;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.textureName = textureName;
        pending_create.width = width;
        pending_create.height = height;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateWorldBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateWorldBrowser(id, url, textureName, width, height);
}

void App::QueueOrCreateWorld2D(int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY)
{
    std::lock_guard<std::mutex> lock(pending_creates_mutex_);
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::World2D;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.width = width;
        pending_create.height = height;

        pending_create.worldX = worldX;
        pending_create.worldY = worldY;
        pending_create.worldZ = worldZ;
        pending_create.offsetZ = offsetZ;
        pending_create.pivotX = pivotX;
        pending_create.pivotY = pivotY;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateWorld2DBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateWorld2DBrowser(id, url, worldX, worldY, worldZ, width, height, offsetZ, pivotX, pivotY);
}

void App::OnPacketReceived(const NetworkPacket& packet)
{
    switch (packet.type)
    {
        case PacketType::ServerConfig:
        {
            const auto& cfg = std::get<ServerConfigPacket>(packet.payload);
            const auto& master_key = cfg.master_resource_key;             if (master_key.size() != 16 && master_key.size() != 24 && master_key.size() != 32)             {                 LOG_ERROR("[CEF] Server master resource key is {} bytes (expected 16, 24 or 32) - resources cannot be decoded.", master_key.size());                 FailPendingCreates("invalid master resource key length");                 break;             } 
            resources_.SetMasterKey(cfg.master_resource_key);
            resources_.SetResourcesLoaderUiEnabled(cfg.resources_loader_ui);
            resources_.MarkAsReadyToDownload();
            break;
        }
        case PacketType::FileData:
        {
            resources_.OnFileData(std::get<FileDataPacket>(packet.payload));
            break;
        }
        case PacketType::EmitEvent:
        {
            const auto& event = std::get<EmitEventPacket>(packet.payload);

            if (event.name == CefEvent::Server::CreateBrowser && event.args.size() >= 4) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                bool focused = event.args[2].boolValue;
                bool controls_chat = event.args[3].boolValue;

                QueueOrCreateOverlay(id, url, focused, controls_chat, -1.f, -1.f);
            }
            else if (event.name == CefEvent::Server::CreateWorldBrowser && event.args.size() >= 5) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                const std::string& textureName = event.args[2].stringValue;
                float width = event.args[3].floatValue;
                float height = event.args[4].floatValue;

                QueueOrCreateWorld(id, url, textureName, width, height);
            }
            else if (event.name == CefEvent::Server::CreateWorld2DBrowser && event.args.size() >= 10) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                float worldX = event.args[2].floatValue;
                float worldY = event.args[3].floatValue;
                float worldZ = event.args[4].floatValue;
                float width = event.args[5].floatValue;
                float height = event.args[6].floatValue;
                float offsetZ = event.args[7].floatValue;
                float pivotX = event.args[8].floatValue;
                float pivotY = event.args[9].floatValue;

                QueueOrCreateWorld2D(id, url, worldX, worldY, worldZ, width, height, offsetZ, pivotX, pivotY);
            } 
            else if (event.name == CefEvent::Server::SetWorld2DBrowserPos && event.args.size() >= 4) {
                int id = event.args[0].intValue;
                float worldX = event.args[1].floatValue;
                float worldY = event.args[2].floatValue;
                float worldZ = event.args[3].floatValue;

                browser_.SetWorld2DBrowserPos(id, worldX, worldY, worldZ);
            }
            else if (event.name == CefEvent::Server::SetBrowserVisible && event.args.size() >= 2) {
                int id = event.args[0].intValue;
                bool visible = event.args[1].boolValue;

                browser_.SetBrowserVisible(id, visible);
            }
            else if (event.name == CefEvent::Server::SetBrowserLayer && event.args.size() == 2 &&
                event.args[0].type == ArgumentType::Integer && event.args[1].type == ArgumentType::Integer) {
                const int id = event.args[0].intValue;
                const int layer = event.args[1].intValue;
                std::lock_guard<std::mutex> lock(pending_creates_mutex_);
                bool pending = false;
                for (auto& create : pending_creates_)
                {
                    if (create.id == id)
                    {
                        create.layer = layer;
                        pending = true;
                    }
                }
                if (!pending)
                    browser_.SetBrowserLayer(id, layer);
            }
            else if (event.name == CefEvent::Server::DestroyBrowser && event.args.size() >= 1) {
                const int id = event.args[0].intValue;

                RemovePendingCreate(id);
                RemovePendingEmits(id);

                browser_.DestroyBrowser(id);
            }
            else if (event.name == CefEvent::Server::ReloadBrowser && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool ignoreCache = event.args[1].boolValue;

                browser_.ReloadBrowser(browserId, ignoreCache);
            }
            else if (event.name == CefEvent::Server::FocusBrowser && event.args.size() >= 2) {
                int browserId = event.args[0].intValue;
                bool toggle = event.args[1].boolValue;

                browser_.FocusBrowser(browserId, toggle);
            }
            else if (event.name == CefEvent::Server::LoadUrl && event.args.size() >= 2)
            {
                const int browserId = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;

                browser_.LoadUrl(browserId, url);
            }
            else if (event.name == CefEvent::Server::AttachBrowserToObject && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int objectId = event.args[1].intValue;

                browser_.AttachBrowserToObject(browserId, objectId);
            }
            else if (event.name == CefEvent::Server::DetachBrowserFromObject && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int objectId = event.args[1].intValue;

                browser_.DetachBrowserFromObject(browserId, objectId);
            }
            else if (event.name == CefEvent::Server::MuteBrowser && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool muted = event.args[1].boolValue;

                audio_.SetStreamMuted(browserId, muted);
            }
            else if (event.name == CefEvent::Server::EnableDevTools && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool enabled  = event.args[1].boolValue;

                browser_.SetDevToolsEnabled(browserId, enabled);
            }
            else if (event.name == CefEvent::Server::SetAudioMode && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int modeValue = event.args[1].intValue;

                AudioMode mode = static_cast<AudioMode>(modeValue);
                audio_.SetStreamAudioMode(browserId, mode);
            }
            else if (event.name == CefEvent::Server::SetAudioSettings && event.args.size() >= 3)
            {
                int browserId = event.args[0].intValue;
                float maxDistance = event.args[1].floatValue;
                float refDistance = event.args[2].floatValue;

                audio_.SetStreamAudioSettings(browserId, maxDistance, refDistance);
            }
            else if (event.name == CefEvent::Server::ToggleHudComponent && event.args.size() >= 2)
            {
                int componentId = event.args[0].intValue;
                bool toggle = event.args[1].boolValue;

                hud_.ToggleComponent(static_cast<EHudComponent>(componentId), toggle);
            }
            else if (event.name == CefEvent::Server::ToggleSpawnScreen && event.args.size() >= 1)
            {
                bool toggle = event.args[0].boolValue;

                hud_.SetClassSelectionVisible(toggle);
            }
            else if (event.name == CefEvent::Server::ClearChat)
            {
                pending_clear_chat_.store(true, std::memory_order_release);
            }
            else if (event.name == CefEvent::Server::ToggleChatInput && event.args.size() >= 1)
            {
                bool toggle = event.args[0].boolValue;

                focus_.SetChatInputEnabled(toggle);
            }
            else if (event.name == CefEvent::Server::SetKeyCapture && event.args.size() >= 1)
            {
                bool enabled = event.args[0].boolValue;

                browser_.SetKeyCaptureEnabled(enabled);
            }
            else if (event.name == CefEvent::Server::EnableKey && event.args.size() >= 2)
            {
                int key = event.args[0].intValue;
                bool enabled = event.args[1].boolValue;

                browser_.EnableKey(key, enabled);
            }
            else if (event.name == CefEvent::Server::ExitGame)
            {
                browser_.ExitGame();
            }
            else if (event.name == CefEvent::Server::SetEscapeMenuMode && event.args.size() >= 1)
            {
                int mode = event.args[0].intValue;

                browser_.SetEscapeMenuMode(static_cast<EscapeMenuMode>(mode));
            }
            else if (event.name == CefEvent::Server::SetPlayerListMode && event.args.size() >= 1)
            {
                int mode = event.args[0].intValue;

                browser_.SetPlayerListMode(static_cast<PlayerListMode>(mode));
            }
            break;
        }
        case PacketType::EmitBrowserEvent:
        {
            const auto& event = std::get<EmitEventPacket>(packet.payload);

            const int browserId = event.browserId;
            const std::string& eventName = event.name;

            if (browser_.HasBrowser(browserId))
            {
				SendEmitToBrowser(browser_, browserId, eventName, event.args);
                LOG_DEBUG("[CEF] EmitEvent {} sent to browser {} with {} args", eventName.c_str(), browserId, event.args.size());
            }
            else
            {
                PendingEmit pending_emit;
                pending_emit.browserId = browserId;
                pending_emit.eventName = eventName;
                pending_emit.args = event.args;

                {
                    std::lock_guard<std::mutex> lock(pending_emits_mutex_);
                    pending_emits_.push_back(std::move(pending_emit));
                }

                LOG_INFO("[CEF] EmitEvent '{}' queued for browser {} (not created yet).", eventName.c_str(), browserId);
            }

            break;
        }
        default:
            break;
    }
}

void App::Shutdown()
{
    LOG_INFO("Shutdown omp-cef client app ...");

    network_.Shutdown();
    resources_.OnDisconnect();
    ResetSession();

    browser_.Shutdown();
    audio_.Shutdown();

    LOG_INFO("Good bye! See you soon.");
}
