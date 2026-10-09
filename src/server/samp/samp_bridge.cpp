#include "samp_bridge.hpp"
#include <common/encoding.hpp>

#ifndef SAMPGDK_STATIC
#define SAMPGDK_STATIC
#endif
#include <sampgdk/core.h>
#include <sampgdk/interop.h>
#include <sampgdk/a_samp.h>
#include <sampgdk/a_players.h>

extern std::vector<AMX*> g_AmxList;

namespace
{
    // amx_PushString only writes its out parameter when the allocation succeeded,
    // so a caller must never use that address as a release point on failure:
    // amx_Release(amx, 0) rewinds the heap pointer into the script's data segment.
    // These helpers report the failure instead, letting the callers skip amx_Exec
    // (a public entered with a missing argument reads a stale stack cell).
    bool PushCell(AMX* amx, cell value)
    {
        return amx_Push(amx, value) == AMX_ERR_NONE;
    }

    bool PushText(AMX* amx, const char* text)
    {
        return amx_PushString(amx, nullptr, nullptr, text, 0, 0) == AMX_ERR_NONE;
    }
}

std::unique_ptr<IPlatformBridge> CreateSampPlatformBridge()
{
    return std::make_unique<SampPlatformBridge>();
}

void SampPlatformBridge::LogInfo(const std::string& message)
{
    sampgdk::logprintf("[CEF] [INFO] %s", message.c_str());
}

void SampPlatformBridge::LogWarn(const std::string& message)
{
    sampgdk::logprintf("[CEF] [WARN] %s", message.c_str());
}

void SampPlatformBridge::LogError(const std::string& message)
{
    sampgdk::logprintf("[CEF] [ERROR] %s", message.c_str());
}

void SampPlatformBridge::LogDebug(const std::string& message)
{
    sampgdk::logprintf("[CEF] [DEBUG] %s", message.c_str());
}

void SampPlatformBridge::CallPawnPublic(const std::string& name, const std::vector<Argument>& args)
{
    for (AMX* amx : g_AmxList)
    {
        int idx = 0;
        if (amx_FindPublic(amx, name.c_str(), &idx) != AMX_ERR_NONE)
            continue;

        // Heap top before any string is allotted: releasing back to this point frees
        // every string cell at once (the AMX heap grows upwards), which is what
        // open.mp's IPawnScript::CallChecked does as well.
        const cell heap_before_push = amx->hea;
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;
        bool push_failed = false;

        for (auto it = args.rbegin(); it != args.rend(); ++it)
        {
            const auto& arg = *it;
            bool pushed = true;

            switch (arg.type)
            {
                case ArgumentType::String:
                {
                    std::string ansi_string = Utf8ToAnsi(arg.stringValue);
                    pushed = PushText(amx, ansi_string.c_str());
                    break;
                }
                case ArgumentType::Integer:
                    pushed = PushCell(amx, arg.intValue);
                    break;
                case ArgumentType::Float:
                    pushed = PushCell(amx, amx_ftoc(arg.floatValue));
                    break;
                case ArgumentType::Bool:
                    pushed = PushCell(amx, arg.boolValue);
                    break;
            }

            if (!pushed)
            {
                push_failed = true;
                LogError("CallPawnPublic(" + name + "): failed to push an argument (script out of memory?), callback skipped.");
                break;
            }
        }

        if (!push_failed)
        {
            cell ret;
            if (amx_Exec(amx, &ret, idx) != AMX_ERR_NONE)
            {
                LogError("CallPawnPublic(" + name + "): amx_Exec failed.");
            }
        }
        else
        {
            // amx_Exec is what consumes the pushed parameters; without it the
            // arguments already pushed would be handed to the next call, so
            // rewind the argument stack to its pre-push state.
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }

        amx_Release(amx, heap_before_push);
    }
}

void SampPlatformBridge::CallOnBrowserCreated(int playerid, int browserId, bool success, int code, const std::string& reason)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefBrowserCreated", &idx) != AMX_ERR_NONE) 
            continue;

        // See CallPawnPublic: never release an address that may not have been allotted.
        const cell heap_before_push = amx->hea;
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;

        bool pushed = true;
        pushed = PushText(amx, reason.c_str()) && pushed;
        pushed = PushCell(amx, code) && pushed;
        pushed = PushCell(amx, success) && pushed;
        pushed = PushCell(amx, browserId) && pushed;
        pushed = PushCell(amx, playerid) && pushed;

        if (pushed)
        {
            cell retval;
            if (amx_Exec(amx, &retval, idx) != AMX_ERR_NONE)
                LogError("CallOnBrowserCreated: amx_Exec failed.");
        }
        else
        {
            LogError("CallOnBrowserCreated: failed to push an argument, callback skipped.");
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }

        amx_Release(amx, heap_before_push);
    }
}

void SampPlatformBridge::CallOnDownloadStart(int playerid)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadStart", &idx) != AMX_ERR_NONE) 
            continue;

        // See the note on PushCell: entering the public after a failed push would let it read
        // whatever is on the stack instead of the arguments it declares.
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;

        if (PushCell(amx, playerid))
        {
            cell retval;
            if (amx_Exec(amx, &retval, idx) != AMX_ERR_NONE)
                LogError("CallOnDownloadStart: amx_Exec failed.");
        }
        else
        {
            LogError("CallOnDownloadStart: failed to push an argument, callback skipped.");
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }
    }
}

void SampPlatformBridge::CallOnDownloadProgress(
    int playerid,
    const std::string& fileName,
    int filePercent,
    int totalPercent,
    int fileDownloadedKb,
    int fileTotalKb,
    int totalDownloadedKb,
    int totalKb)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadProgress", &idx) != AMX_ERR_NONE)
            continue;

        // See CallPawnPublic: never release an address that may not have been allotted.
        const cell heap_before_push = amx->hea;
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;

        std::string ansi_file_name = Utf8ToAnsi(fileName);

        bool pushed = true;
        pushed = PushCell(amx, totalKb) && pushed;
        pushed = PushCell(amx, totalDownloadedKb) && pushed;
        pushed = PushCell(amx, fileTotalKb) && pushed;
        pushed = PushCell(amx, fileDownloadedKb) && pushed;
        pushed = PushCell(amx, totalPercent) && pushed;
        pushed = PushCell(amx, filePercent) && pushed;
        pushed = PushText(amx, ansi_file_name.c_str()) && pushed;
        pushed = PushCell(amx, playerid) && pushed;

        if (pushed)
        {
            cell retval;
            if (amx_Exec(amx, &retval, idx) != AMX_ERR_NONE)
                LogError("CallOnDownloadProgress: amx_Exec failed.");
        }
        else
        {
            LogError("CallOnDownloadProgress: failed to push an argument, callback skipped.");
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }

        amx_Release(amx, heap_before_push);
    }
}

void SampPlatformBridge::CallOnDownloadFinish(int playerid)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadFinish", &idx) != AMX_ERR_NONE) 
            continue;

        // See the note on PushCell: entering the public after a failed push would let it read
        // whatever is on the stack instead of the arguments it declares.
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;

        if (PushCell(amx, playerid))
        {
            cell retval;
            if (amx_Exec(amx, &retval, idx) != AMX_ERR_NONE)
                LogError("CallOnDownloadFinish: amx_Exec failed.");
        }
        else
        {
            LogError("CallOnDownloadFinish: failed to push an argument, callback skipped.");
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }
    }
}

void SampPlatformBridge::CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefPressKey", &idx) != AMX_ERR_NONE) 
            continue;

        // See the note on PushCell: entering the public after a failed push would let it read
        // whatever is on the stack instead of the arguments it declares.
        const cell stk_before_push = amx->stk;
        const int params_before_push = amx->paramcount;

        bool pushed = true;
        pushed = PushCell(amx, repeat) && pushed;
        pushed = PushCell(amx, down) && pushed;
        pushed = PushCell(amx, modifiers) && pushed;
        pushed = PushCell(amx, scancode) && pushed;
        pushed = PushCell(amx, key) && pushed;
        pushed = PushCell(amx, playerid) && pushed;

        if (pushed)
        {
            cell retval;
            if (amx_Exec(amx, &retval, idx) != AMX_ERR_NONE)
                LogError("CallOnPressKey: amx_Exec failed.");
        }
        else
        {
            LogError("CallOnPressKey: failed to push an argument, callback skipped.");
            amx->stk = stk_before_push;
            amx->paramcount = params_before_push;
        }
    }
}

void SampPlatformBridge::ShowResourceDownloadDialog(
    int playerid,
    int dialogid,
    const std::string& title,
    const std::string& body,
    const std::string& button1,
    const std::string& button2)
{
    static constexpr int DialogStyleTablistHeaders = 5;

    const std::string ansi_title = Utf8ToAnsi(title);
    const std::string ansi_body = Utf8ToAnsi(body);
    const std::string ansi_button1 = Utf8ToAnsi(button1);
    const std::string ansi_button2 = Utf8ToAnsi(button2);

    sampgdk_ShowPlayerDialog(
        playerid,
        dialogid,
        DialogStyleTablistHeaders,
        ansi_title.c_str(),
        ansi_body.c_str(),
        ansi_button1.c_str(),
        ansi_button2.c_str());
}

void SampPlatformBridge::HideResourceDownloadDialog(int playerid)
{
    static constexpr int DialogStyleMsgBox = 0;

    sampgdk_ShowPlayerDialog(playerid, -1, DialogStyleMsgBox, " ", " ", " ", " ");
}

std::string SampPlatformBridge::GetPlayerAddressIp(int playerid)
{
    char ip[64] = {};
    if (sampgdk_GetPlayerIp(playerid, ip, sizeof(ip)) == 0)
        return std::string(ip);

    return {};
}

void SampPlatformBridge::KickPlayer(int playerid)
{
    sampgdk_Kick(playerid);
}

bool SampPlatformBridge::IsPlayerNpcBot(int playerid)
{
    return sampgdk_IsPlayerNPC(playerid);
}
