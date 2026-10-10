/*
 * C++ event handler interface for downstream components.
 */

#pragma once

#include <cstdint>
#include <vector>

enum class CefArgType : uint8_t
{
    String = 0,
    Integer = 1,
    Float = 2,
    Bool = 3,
};

struct CefArg
{
    CefArgType type;
    const char* stringValue;
    int intValue;
    float floatValue;
    bool boolValue;
};

struct ICefEventHandler
{
    virtual ~ICefEventHandler() = default;

    virtual void onCefInitialize(int playerid, bool success, int reason, const char* message) {}
    virtual void onCefReady(int playerid) {}
    virtual void onCefBrowserCreated(int playerid, int browserId, bool success, int code, const char* reason) {}
    virtual void onCefDownloadStart(int playerid) {}
    virtual void onCefDownloadProgress(
        int playerid,
        const char* fileName,
        int filePercent,
        int totalPercent,
        int fileDownloadedKb,
        int fileTotalKb,
        int totalDownloadedKb,
        int totalKb) {}
    virtual void onCefDownloadFinish(int playerid) {}
    virtual void onCefPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat) {}
    virtual void onCefChatInputState(int playerid, bool open) {}
    virtual void onCefEvent(int playerid, int browserId, const char* name, int argCount, const CefArg* args) {}
};

// Shared handler list, defined in cef_event_handlers.cpp. Both plugin.cpp
// (fires callbacks) and cef_extension_api.cpp (registers handlers) touch it.
std::vector<ICefEventHandler*>& CefHandlerList();
// Returned by value on purpose: a handler may register or unregister handlers while it is being
// dispatched, which would invalidate an iterator into the shared list.
std::vector<ICefEventHandler*> GetCefEventHandlers();
