#ifndef CTRPLUGINFRAMEWORK_MENU_HPP
#define CTRPLUGINFRAMEWORK_MENU_HPP

#include "CTRPluginFramework.hpp"
#include "MenuItem.hpp"
#include "MenuFolder.hpp"
#include "MenuEntry.hpp"

namespace CTRPluginFramework
{
    class Menu
    {
        struct MenuFlags
        {
            bool showFPS : 1;
        };

    public:

        Menu(std::string name = "Cheats", std::string note = "");
        ~Menu(void);

        void    Append(MenuItem *item);
        int     Run(void);
    private:

        void    _RenderTop(void);
        void    _RenderBottom(void);
        void    _ProcessEvent(Event &event);
        void    _Update(Time delta);

        std::vector<MenuEntry *>    _executeLoop;
        std::queue<int>             _freeIndex;
        MenuFolder                  *_folder;
        MenuFolder                  *_starred;
        bool                        _isOpen;
        bool                        _starMode;

        int                         _selector;
        int                         _selectedTextSize;
        float                       _maxScrollOffset;
        float                       _scrollOffset;
        Clock                       _scrollClock;
        bool                        _reverseFlow;
    };
}

#endif