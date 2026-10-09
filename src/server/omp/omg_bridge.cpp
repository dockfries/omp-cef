#include "omp_bridge.hpp"
#include <sdk.hpp>
#include <common/encoding.hpp>

OmpPlatformBridge::OmpPlatformBridge(ICore* core, IPawnComponent* pawn) : core_(core), pawn_(pawn) {}

std::unique_ptr<IPlatformBridge> CreateOmpPlatformBridge(ICore* core, IPawnComponent* pawn)
{
    return std::make_unique<OmpPlatformBridge>(core, pawn);
}

void OmpPlatformBridge::LogInfo(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] %s", message.c_str());
}

void OmpPlatformBridge::LogWarn(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [WARN] %s", message.c_str());
}

void OmpPlatformBridge::LogError(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [ERROR] %s", message.c_str());
}

void OmpPlatformBridge::LogDebug(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [DEBUG] %s", message.c_str());
}

void OmpPlatformBridge::CallPawnPublic(const std::string& name, const std::vector<Argument>& args)
{
    if (!pawn_)
        return;

    auto call_on_script = [&](IPawnScript* script)
    {
        if (!script)
            return;

        int idx = 0;
        if (script->FindPublic(name.c_str(), &idx) != AMX_ERR_NONE)
            return;

        // String arguments are allotted on the script's heap and must be released
        // by the caller once the public returns, as IPawnScript::CallChecked does.
        // Releasing the heap top taken before the pushes frees all of them at once.
        cell heap_before_push = script->GetHEA();

        for (auto it = args.rbegin(); it != args.rend(); ++it)
        {
            const auto& arg = *it;
            switch (arg.type)
            {
                case ArgumentType::String:
                {
                    std::string ansi_string = Utf8ToAnsi(arg.stringValue);
                    script->PushString(nullptr, nullptr, ansi_string, false, false);
                    break;
                }
                case ArgumentType::Integer:
                    script->Push(arg.intValue);
                    break;
                case ArgumentType::Float:
                    script->Push(amx_ftoc(arg.floatValue));
                    break;
                case ArgumentType::Bool:
                    script->Push(arg.boolValue);
                    break;
            }
        }

        cell retval;
        script->Exec(&retval, idx);
        script->Release(heap_before_push);
    };

    // Snapshot the component for the duration of the dispatch: any callback below
    // can free the Pawn component (onFree nulls pawn_, and its scripts with it),
    // so pawn_->sideScripts() after a callback would dereference null.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call_on_script(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts())
    {
        call_on_script(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::CallOnBrowserCreated(int playerid, int browserId, bool success, int code, const std::string& reason)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefBrowserCreated", DefaultReturnValue_False, playerid, browserId, success, code, StringView(reason));
    };

    // See CallPawnPublic: a callback can free the Pawn component, so work from a
    // snapshot and stop as soon as it is gone.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts()) {
        call(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::CallOnDownloadStart(int playerid)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefDownloadStart", DefaultReturnValue_True, playerid);
    };

    // See CallPawnPublic: a callback can free the Pawn component.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts()) {
        call(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::CallOnDownloadProgress(
    int playerid,
    const std::string& fileName,
    int filePercent,
    int totalPercent,
    int fileDownloadedKb,
    int fileTotalKb,
    int totalDownloadedKb,
    int totalKb)
{
    if (!pawn_)
        return;

    auto call = [&](IPawnScript* script) {
        if (!script)
            return;

        script->Call(
            "OnCefDownloadProgress",
            DefaultReturnValue_True,
            playerid,
            StringView(fileName),
            filePercent,
            totalPercent,
            fileDownloadedKb,
            fileTotalKb,
            totalDownloadedKb,
            totalKb);
    };

    // See CallPawnPublic: a callback can free the Pawn component.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts()) {
        call(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::CallOnDownloadFinish(int playerid)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefDownloadFinish", DefaultReturnValue_True, playerid);
    };

    // See CallPawnPublic: a callback can free the Pawn component.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts()) {
        call(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefPressKey", DefaultReturnValue_True, playerid, key, scancode, modifiers, down, repeat);
    };

    // See CallPawnPublic: a callback can free the Pawn component.
    IPawnComponent* const pawn = pawn_;
    if (!pawn)
        return;

    call(pawn->mainScript());

    if (pawn_ != pawn)
        return;

    for (IPawnScript* script : pawn->sideScripts()) {
        call(script);

        if (pawn_ != pawn)
            return;
    }
}

void OmpPlatformBridge::ShowResourceDownloadDialog(
    int playerid,
    int dialogid,
    const std::string& title,
    const std::string& body,
    const std::string& button1,
    const std::string& button2)
{
    if (!core_)
        return;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return;

    auto* dialog = queryExtension<IPlayerDialogData>(*player);
    if (!dialog)
        return;

    dialog->show(
        *player,
        dialogid,
        DialogStyle_TABLIST_HEADERS,
        StringView(title),
        StringView(body),
        StringView(button1),
        StringView(button2));
}

void OmpPlatformBridge::HideResourceDownloadDialog(int playerid)
{
    if (!core_)
        return;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return;

    auto* dialog = queryExtension<IPlayerDialogData>(*player);
    if (!dialog)
        return;

    dialog->hide(*player);
}

std::string OmpPlatformBridge::GetPlayerAddressIp(int playerid)
{
    if (core_)
    {
        IPlayer* player = core_->getPlayers().get(playerid);
        if (player)
        {
            const PeerAddress& addr = player->getNetworkData().networkID.address;

            PeerAddress::AddressString address_str;
            if (PeerAddress::ToString(addr, address_str))
            {
                return std::string(address_str.data());
            }
        }
    }

    return "";
}

void OmpPlatformBridge::KickPlayer(int playerid)
{
    if (core_)
    {
        IPlayer* player = core_->getPlayers().get(playerid);
        if (!player)
            return;

        player->kick();
    }
}

bool OmpPlatformBridge::IsPlayerNpcBot(int playerid)
{
    if (!core_)
        return false;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return false;

    return player->isBot();
}

void OmpPlatformBridge::InvalidatePawn()
{
	pawn_ = nullptr;
}