#pragma once

#include <memory>
#include <utility>
#include <vector>

// Objects that have to survive the shutdown path.
//
// Everything the client owns is destroyed from DllMain(DLL_PROCESS_DETACH), i.e. while the loader
// lock is held. Destroying an object that owns a thread, a D3D or CEF handle, or RenderWare
// resources from there can block the exit forever: the thread we would wait for may need the same
// loader lock, and the render thread that is supposed to free the GPU objects is already gone.
// Those objects are handed over here instead. The process is exiting, so the OS reclaims them -
// the same reason CefShutdown() is never called.
namespace exit_lifetime
{
    inline std::vector<std::shared_ptr<void>>& AbandonedResources()
    {
        // Deliberately leaked: a plain static would be destroyed during CRT termination, which is
        // exactly the situation this exists to avoid.
        static std::vector<std::shared_ptr<void>>* abandoned = new std::vector<std::shared_ptr<void>>();
        return *abandoned;
    }

    inline void Abandon(std::shared_ptr<void> resource)
    {
        if (resource)
            AbandonedResources().push_back(std::move(resource));
    }

    template <typename T>
    inline void Abandon(std::unique_ptr<T> resource)
    {
        // One list per type; the objects are never destroyed, they only have to stay alive.
        static std::vector<std::unique_ptr<T>>* abandoned = new std::vector<std::unique_ptr<T>>();

        if (resource)
            abandoned->push_back(std::move(resource));
    }
}
