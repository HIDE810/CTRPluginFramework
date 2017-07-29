#ifndef CTRPLUGINFRAMEWORKIMPL_PLUGINMENUHOME_HPP
#define CTRPLUGINFRAMEWORKIMPL_PLUGINMENUHOME_HPP

#include "CTRPluginFrameworkImpl/Graphics.hpp"
#include "CTRPluginFrameworkImpl/System.hpp"
#include "CTRPluginFrameworkImpl/Menu/MenuFolderImpl.hpp"
#include "CTRPluginFrameworkImpl/Menu/MenuEntryImpl.hpp"
#include "CTRPluginFrameworkImpl/Menu/MenuItem.hpp"

#include <vector>
namespace CTRPluginFramework
{
    class PluginMenuHome
    {
        using EventList = std::vector<Event>;
    public:
        PluginMenuHome(std::string &name);
        ~PluginMenuHome(){}

        // Return true if the Close Button is pressed, else false
        bool    operator()(EventList &eventList, int &mode, Time &delta);
        void    Append(MenuItem *item) const;
        void    Refresh(void);
        void    UnStar(MenuItem* item);
        void    Init(void);

        void    TriggerSearch(bool state);
        void    TriggerActionReplay(bool state);
        void    AddPluginVersion(u32 version);

    private:
        friend class PluginMenuImpl;

        void    _ProcessEvent(Event &event);
        void    _RenderTop(void);
        void    _RenderBottom(void);
        void    _Update(Time delta);

        void    _DisplayNote(void);
        void    _StarItem(void);
        void    _TriggerEntry(void);


        // Members
        MenuFolderImpl                  *_root;
        MenuFolderImpl                  *_folder;
        MenuFolderImpl                  *_starred;        
        MenuFolderImpl                  *_starredConst;

        bool                        _starMode;
        int                         _selector;
        int                         _selectedTextSize;
        float                       _maxScrollOffset;
        float                       _scrollOffset;
        Clock                       _scrollClock;
        bool                        _reverseFlow;
        bool                        _showVersion;
        int                         _versionPosX;
        std::string                 _versionStr;

        TextBox                     *_noteTB;

        // Mode buttons
        CheckedButton<PluginMenuHome, void>   _showStarredBtn;        
        Button<PluginMenuHome, void>          _hidMapperBtn;
        Button<PluginMenuHome, void>          _gameGuideBtn;        
        Button<PluginMenuHome, void>          _searchBtn;
        Button<PluginMenuHome, void>          _arBtn;       
        Button<PluginMenuHome, void>          _toolsBtn;
        
        IconButton<PluginMenuHome, void>     _closeBtn;
        IconButton<PluginMenuHome, void>     _keyboardBtn;
        IconButton<PluginMenuHome, void>     _controllerBtn;

        // Entry button
        ToggleButton<PluginMenuHome, void>      _AddFavoriteBtn;
        ToggleButton<PluginMenuHome, void>      _InfoBtn;

    };
}

#endif