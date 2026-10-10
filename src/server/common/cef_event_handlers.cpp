#include "cef_event_handlers.hpp"

std::vector<ICefEventHandler*>& CefHandlerList()
{
    static std::vector<ICefEventHandler*> s;
    return s;
}

std::vector<ICefEventHandler*> GetCefEventHandlers()
{
    // A copy: a handler that registers or unregisters handlers while it is being dispatched would
    // otherwise invalidate the iteration (erase shifts the elements, push_back can reallocate).
    return CefHandlerList();
}
