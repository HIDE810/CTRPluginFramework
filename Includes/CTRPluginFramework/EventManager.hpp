#ifndef CTRPLUGINFRAMEWORK_EVENTMANAGER_HPP
#define CTRPLUGINFRAMEWORK_EVENTMANAGER_HPP

#include "types.h"
#include "ctrulib/services/hid.h"
#include "Events.hpp"
#include <queue>

namespace CTRPluginFramework
{
    class EventManager
    {
    public:
        EventManager(void);

        bool PollEvent(Event &event);
        bool WaitEvent(Event &event);

    private:

        bool PopEvent(Event &event, bool isBlocking);
        void PushEvent(const Event &event);
        void ProcessEvents(void);

        std::queue<Event>   _eventsQueue;
        touchPosition       _lastTouch;
        float               _slider3D;
        u32                 _keysHeld;

    };
}

#endif