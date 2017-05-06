#include "CTRPluginFramework/Menu.hpp"
#include "CTRPluginFrameworkImpl/Menu.hpp"
#include "CTRPluginFrameworkImpl/Menu/PluginMenuImpl.hpp"

namespace CTRPluginFramework
{
    PluginMenu::PluginMenu(std::string name, std::string note) :
        _menu(new PluginMenuImpl(name, note)) 
    {

    }

    PluginMenu::~PluginMenu(void)
    {
    }

    void    PluginMenu::Append(MenuEntry *item) const
    {
        if (item == nullptr)
            return;

        MenuEntryImpl *entry = item->_item.get();
        _menu->Append(entry);
    }

    void    PluginMenu::Append(MenuFolder *item) const
    {
        if (item == nullptr)
            return;

        MenuFolderImpl *folder = item->_item.get();
        _menu->Append(folder);
    }

    void    PluginMenu::Callback(CallbackPointer callback) const
    {
        if (callback != nullptr)
            _menu->Callback(callback);
    }

    int    PluginMenu::Run(void) const
    {
       return (_menu->Run());
    }

    void    PluginMenu::SetSearchButtonState(bool isEnabled) const
    {
        _menu->TriggerSearch(isEnabled);
    }

    void    PluginMenu::SetActionReplayButtonState(bool isEnabled) const
    {
        return;
        _menu->TriggerActionReplay(isEnabled);
    }
}
