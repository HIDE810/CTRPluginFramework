#include "3DS.h"
#include <stdlib.h>
#include <cstdio>
#include "CTRPluginFrameworkImpl/arm11kCommands.h"
#include "CTRPluginFrameworkImpl.hpp"
#include "CTRPluginFramework.hpp"
#include "CTRPluginFrameworkImpl/Graphics/Font.hpp"
#include "NTRImpl.hpp"

extern "C" void     abort(void);
extern "C" void     initSystem();
extern "C" void     initLib();

namespace CTRPluginFramework
{
    void  ThreadExit(void) __attribute__((noreturn));
}

void abort(void)
{
    CTRPluginFramework::Color c(255, 69, 0); //red(255, 0, 0);
    CTRPluginFramework::Screen::Top->Flash(c);
    CTRPluginFramework::Screen::Bottom->Flash(c);

    CTRPluginFramework::ThreadExit();
}

namespace CTRPluginFramework
{
    // Threads stacks
    static u32  threadStack[0x4000] ALIGN(8);
    static u32  keepThreadStack[0x1000] ALIGN(8);

    // Some globals
    Handle      g_continueGameEvent = 0;
    Handle      g_keepThreadHandle;
    Handle      g_keepEvent = 0;
    bool        g_keepRunning = true;
    Thread      g_mainThread = nullptr;

    void    ThreadInit(void *arg);

    // From main.cpp
    void    PatchProcess(void);
    int     main(void);
    
    // allocateHeaps.cpp
    extern u32      g_linearOp;
    extern bool     g_heapError;

    extern "C" void __appInit(void);

    extern "C" Result __sync_init(void);
    extern "C" void __system_initSyscalls(void);

    void    KeepThreadMain(void *arg)
    {
        // Initialize the synchronization subsystem
        __sync_init();

        // Initialize newlib's syscalls
        __system_initSyscalls();
        
        // Initialize services
        srvInit();
        acInit();
        amInit();
        fsInit();
        cfguInit();

        // Init Framework's system constants
        SystemImpl::Initialize();

        // Init Process info
        ProcessImpl::Initialize();

        // Init Screen
        Screen::Initialize();

        // Patch process before it starts
        PatchProcess();

        // Continue game
        svcSignalEvent(g_continueGameEvent);

        // Correction for some games like Kirby
        u64 tid = Process::GetTitleID();

        // Pokemon Sun & Moon
        if (tid == 0x0004000000175E00 || tid == 0x0004000000164800
        // ACNL
        ||  tid == 0x0004000000086300 || tid == 0x0004000000086400 || tid == 0x0004000000086200
        // Mario Kart
       // || tid == 0x0004000000030600  || tid == 0x0004000000030700 || tid == 0x0004000000030800)
        )
            g_linearOp = 0x10203u;

        // Wait for the game to be fully launched
        Sleep(Seconds(5));

        // Init heap and newlib's syscalls
        initLib();
        //initSystem();

        // If heap error, exit
        if (g_heapError)
            goto exit;

        // Create plugin's main thread
        g_mainThread = threadCreate(ThreadInit, (void *)threadStack, 0x4000, 0x18, -2, false);

        svcCreateEvent(&g_keepEvent, RESET_ONESHOT);

        while (g_keepRunning)
        {
            svcWaitSynchronization(g_keepEvent, U64_MAX); 
            svcClearEvent(g_keepEvent);

            while (ProcessImpl::IsPaused())
            {
                if (ProcessImpl::IsAcquiring())
                    Sleep(Milliseconds(100));
            }               
        }

        threadJoin(g_mainThread, U64_MAX);
    exit:
        exit(1);
    }

    extern "C" vu32* hidSharedMem;
    FS_Archive  _sdmcArchive;

    void    InitColors(void);
    void    Initialize(void)
    {
        // Init HID
        hidInit();

        // Init classes
        Renderer::Initialize();
        Font::Initialize();

        // could probably get swapped for lighter implement of gspevent init
        gfxInit(Screen::Top->GetFormat(), Screen::Bottom->GetFormat(), false);

        // Init Process info
        ProcessImpl::UpdateThreadHandle();

        //Init OSD
        OSDImpl::_Initialize();  

        // Init NTR
        NTRImpl::InitNTR();

        // Init Colors
        InitColors();

        // Init sdmcArchive
        {
            FS_Path sdmcPath = { PATH_EMPTY, 1, (u8*)"" };
            FSUSER_OpenArchive(&_sdmcArchive, ARCHIVE_SDMC, sdmcPath);
        }
        // Set current working directory
        {
            u64     tid = Process::GetTitleID();
            char    path[256] = { 0 };

            sprintf(path, "/plugin/%016llX/", tid);
            Directory::ChangeWorkingDirectory(path);
        }

        // Protect VRAM
        Process::ProtectRegion(0x1F000000, 3);

        // Protect HID Shared Memory in case we want to push inputs
        Process::ProtectMemory((u32)hidSharedMem, 0x1000);      
    }
    void    InitializeRandomEngine(void);
    // Main thread's start
    void  ThreadInit(void *arg)
    {
        CTRPluginFramework::Initialize();

        // Reduce Priority
        ProcessImpl::Play(false);

        // Initialize Globals settings
        Preferences::Initialize();    
        Window::Initialize();
        InitializeRandomEngine();

        // Start plugin
        int ret = main();

        ThreadExit();
    }

    void  ThreadExit(void)
    {
        // In which thread are we ?
        if (threadGetCurrent() != nullptr)
        {
            // MainThread

            // Release process in case it's currently paused
            ProcessImpl::Play(false);

            // Exit services
            gfxExit();

            // Exit loop in keep thread
            g_keepRunning = false;
            svcSignalEvent(g_keepEvent);

            threadExit(1);
        }
        else
        {
            // KeepThread
            if (g_mainThread != nullptr)
            {
                ProcessImpl::Play(false);
                PluginMenuImpl::ForceExit();
                threadJoin(g_mainThread, U64_MAX);
            }
            else
                svcSignalEvent(g_continueGameEvent);

            // Exit services
            acExit();
            amExit();
            fsExit();
            cfguExit();            

            exit(-1);
        }
    }

    extern "C" int LaunchMainThread(int arg);
    int   LaunchMainThread(int arg)
    {
        svcCreateEvent(&g_continueGameEvent, RESET_ONESHOT);
        svcCreateThread(&g_keepThreadHandle, KeepThreadMain, 0, &keepThreadStack[0x1000], 0x1A, -2);
        svcWaitSynchronization(g_continueGameEvent, U64_MAX);
        return (0);
    }

}
