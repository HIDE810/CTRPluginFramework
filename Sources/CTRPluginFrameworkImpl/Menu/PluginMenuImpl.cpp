#include "types.h"
#include "ctrulib/services/gspgpu.h"

#include <string>
#include <vector>
#include <cstdio>

#include "CTRPluginFrameworkImpl/Graphics.hpp"
#include "CTRPluginFramework/Graphics.hpp"

#include "CTRPluginFrameworkImpl/Menu.hpp"
#include "CTRPluginFrameworkImpl/System.hpp"
#include "CTRPluginFramework/System.hpp"
#include "CTRPluginFrameworkImpl/Preferences.hpp"

namespace CTRPluginFramework
{
    PluginMenuImpl  *PluginMenuImpl::_runningInstance = nullptr;

    PluginMenuImpl::PluginMenuImpl(std::string &name, std::string &about) : 
    _hexEditor(0x00100000),
    _freeCheats(_hexEditor),
    _home(new PluginMenuHome(name)),
    _search(new PluginMenuSearch(_hexEditor, _freeCheats)),
    _tools(new PluginMenuTools(about, _hexEditor, _freeCheats)),
    _executeLoop(new PluginMenuExecuteLoop()),
    _guide(new GuideReader())
    {
        _isOpen = false;
        _wasOpened = false;
        _pluginRun = true;
    }

    PluginMenuImpl::~PluginMenuImpl(void)
    {
        ProcessImpl::Play(false);
        delete _home;
        delete _search;
        delete _tools;
        delete _executeLoop;
        delete _guide;
    }  

    void    PluginMenuImpl::Append(MenuItem *item) const
    {
        _home->Append(item);
    }

    void    PluginMenuImpl::Callback(CallbackPointer callback)
    {
        _callbacks.push_back(callback);
    }

    /*
    ** Run
    **************/

    int    PluginMenuImpl::Run(void)
    {
        Event                   event;
        EventManager            manager;
        Clock                   clock;
        Clock                   inputClock;
        int                     mode = 0;
        bool                    shouldClose = false;

        // Component
        PluginMenuHome          &home = *_home;
        PluginMenuTools         &tools = *_tools;
        PluginMenuSearch        &search = *_search;
        GuideReader             &guide = *_guide;
        PluginMenuExecuteLoop   &executer = *_executeLoop;

        Time            delta;
        OSDImpl         &osd = *(OSDImpl::GetInstance());
        std::vector<Event>     eventList;

        // Set _runningInstance to this menu
        _runningInstance = this;

        // Load settings
        Preferences::LoadSettings();
        _tools->UpdateSettings();

        // Load favorites and enabled cheats
        if (Preferences::AutoLoadFavorites)
            Preferences::LoadSavedFavorites();
        if (Preferences::AutoLoadCheats)
            Preferences::LoadSavedEnabledCheats();

        // Load FreeCheats
        Preferences::LoadFreeCheats();

        // Update PluginMenuHome variables
        home.Init();

        OSD::Notify("Plugin ready !", Color(255, 255, 255), Color());

        // Main loop
        while (_pluginRun)
        {
            // Check Event
            eventList.clear();
            while (manager.PollEvent(event))
            {
                // If it's a KeyPressed event
                if (event.type == Event::KeyPressed && inputClock.HasTimePassed(Milliseconds(500)))
                {
                    bool isHotkeysDown = false;

                    // Check that MenuHotkeys are pressed
                    for (int i = 0; i < 16; i++)
                    {
                        u32 key = Preferences::MenuHotkeys & (1u << i);
                        
                        if (key && event.key.code == key)
                        {
                            if (Controller::IsKeysDown(Preferences::MenuHotkeys ^ key))
                                isHotkeysDown = true;
                        }
                    }

                    // If MenuHotkeys are pressed
                    if (isHotkeysDown)
                    {
                        if (_isOpen)
                        {
                            ProcessImpl::Play(true);
                            _isOpen = false;
                            if (Preferences::InjectBOnMenuClose)
                                Controller::InjectKey(Key::B);

                            // Save settings
                            Preferences::WriteSettings();
                        }
                        else
                        {
                            ProcessImpl::Pause(true);
                            _isOpen = true;
                            _wasOpened = true;
                        }
                        inputClock.Restart();
                    }                       
                }

                if (_isOpen)
                {
                    eventList.push_back(event);
                }
            }
            
            if (_isOpen)
            {   

                if (mode == 0)
                { /* Home */
                    shouldClose = home(eventList, mode, delta);
                }
                /*
                else if (mode == 1)
                { /* Mapper *
    
                }
                */
                else if (mode == 2)
                { /* Guide */
                    if (guide(eventList, delta))
                        mode = 0;
                }
                else if (mode == 3)
                { /* Search */
                    if (search(eventList, delta))
                        mode = 0;
                }                
                /*
                else if (mode == 4)
                { /* ActionReplay  *
    
                }
                */
                else if (mode == 5)
                { /* Tools  */
                    if (tools(eventList, delta))
                        mode = 0;
                }

                // End frame
                Renderer::EndFrame(shouldClose);
                delta = clock.Restart();

                // Close menu
                if (shouldClose)
                {
                    ProcessImpl::Play(true);
                    _isOpen = false;
                    shouldClose = false;
                    if (Preferences::InjectBOnMenuClose)
                        Controller::InjectKey(Key::B);

                    // Save settings
                    Preferences::WriteSettings();
                }    

               /* if (Controller::IsKeysDown((L + R + Start)))
                {
                    ProcessImpl::Play(true);
                    _pluginRun = false;                    
                    _isOpen = false;     
                } */
            }
            else
            {
                // Execute activated cheats
                executer();

                // Execute callbacks
                for (int i = 0; i < _callbacks.size(); i++)
                {
                    _callbacks[i]();
                }
                
                // Display notifications
                osd();
                if (_wasOpened)
                    _wasOpened = false;
            }
        }

        // Remove Running Instance
        _runningInstance = nullptr;
        return (0);
    }

    void PluginMenuImpl::LoadEnabledCheatsFromFile(const Preferences::Header &header, File &settings)
    {
        if (_runningInstance == nullptr)
            return;

        std::vector<u32>    uids;
        MenuFolderImpl      *folder = _runningInstance->_home->_folder;

        uids.resize(header.enabledCheatsCount);

        settings.Seek(header.enabledCheatsOffset, File::SET);

        if (settings.Read(uids.data(), sizeof(u32) * header.enabledCheatsCount) == 0)
        {
            for (u32 &uid : uids)
            {
                MenuItem *item = folder->GetItem(uid);

                if (item != nullptr && item->IsEntry())
                    reinterpret_cast<MenuEntryImpl *>(item)->Enable();
            }
        }
    }

    void PluginMenuImpl::LoadFavoritesFromFile(const Preferences::Header &header, File &settings)
    {
        if (_runningInstance == nullptr)
            return;

        std::vector<u32>    uids;
        MenuFolderImpl      *folder = _runningInstance->_home->_folder;
        MenuFolderImpl      *starred = _runningInstance->_home->_starredConst;

        uids.resize(header.favoritesCount);

        settings.Seek(header.favoritesOffset, File::SET);

        if (settings.Read(uids.data(), sizeof(u32) * header.favoritesCount) == 0)
        {
            for (u32 &uid : uids)
            {
                MenuItem *item = folder->GetItem(uid);

                if (item != nullptr && !item->_IsStarred())
                {
                    item->_TriggerStar();                   
                    starred->Append(item, true);                    
                }
            }
        }
    }

    void    PluginMenuImpl::WriteEnabledCheatsToFile(Preferences::Header &header, File &settings)
    {
        if (_runningInstance == nullptr)
            return;

        std::vector<u32>    uids;
        MenuFolderImpl      *folder = _runningInstance->_home->_folder;

        for (MenuItem *item : folder->_items)
        {
            if (item->IsEntry() && reinterpret_cast<MenuEntryImpl *>(item)->IsActivated())
                uids.push_back(item->_uid);
        }

        if (uids.size())
        {                
            u64 offset = settings.Tell();

            if (settings.Write(uids.data(), sizeof(u32) * uids.size()) == 0)
            {
                header.enabledCheatsCount = uids.size();
                header.enabledCheatsOffset = offset;
            }
        }
    }

    void    PluginMenuImpl::WriteFavoritesToFile(Preferences::Header &header, File &settings)
    {
        if (_runningInstance == nullptr)
            return;

        std::vector<u32>    uids;
        MenuFolderImpl      *folder = _runningInstance->_home->_starred;

        for (MenuItem *item : folder->_items)
        {
            uids.push_back(item->_uid);
        }

        if (uids.size())
        {
            u64 offset = settings.Tell();

            if (settings.Write(uids.data(), sizeof(u32) * uids.size()) == 0)
            {
                header.favoritesCount = uids.size();
                header.favoritesOffset = offset;
            }
        }
    }

    void    PluginMenuImpl::GetRegionsList(std::vector<Region>& list)
    {
        if (_runningInstance != nullptr)
            _runningInstance->_search->GetRegionsList(list);
    }

    void    PluginMenuImpl::ForceExit(void)
    {
        if (_runningInstance != nullptr)
            _runningInstance->_pluginRun = false;
    }

    void    PluginMenuImpl::UnStar(MenuItem* item)
    {
        if (_runningInstance != nullptr)
        {
            _runningInstance->_home->UnStar(item);
        }
    }

    void    PluginMenuImpl::Refresh(void)
    {
        if (_runningInstance != nullptr)
        {
            _runningInstance->_home->Refresh();
        }
    }

    void    PluginMenuImpl::TriggerSearch(bool state) const
    {
        _home->TriggerSearch(state);
    }

    void    PluginMenuImpl::TriggerActionReplay(bool state) const
    {
        _home->TriggerActionReplay(state);
    }

    void    PluginMenuImpl::TriggerFreeCheats(bool isEnabled) const
    {
        _tools->TriggerFreeCheatsEntry(isEnabled);
    }

    bool    PluginMenuImpl::IsOpen(void) const
    {
        return (_isOpen);
    }

    bool    PluginMenuImpl::WasOpened(void) const
    {
        return (_wasOpened);
    }
}
