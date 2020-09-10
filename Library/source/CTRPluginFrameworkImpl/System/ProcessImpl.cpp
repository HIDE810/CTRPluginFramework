#include <3ds.h>

// Fix std::vector<MemInfo> == operator
static bool      operator==(const MemInfo left, const MemInfo right)
{
    return left.base_addr == right.base_addr && left.size == right.size;
}

#include "CTRPluginFramework/System.hpp"
#include "CTRPluginFrameworkImpl/System.hpp"
#include "CTRPluginFrameworkImpl/Preferences.hpp"
#include "CTRPluginFrameworkImpl/Graphics/OSDImpl.hpp"
#include "CTRPluginFrameworkImpl/System/Services/Gsp.hpp"
#include "CTRPluginFramework/Utils/Utils.hpp"

#include <cstdio>
#include <cstring>
#include "csvc.h"

extern      Handle gspThreadEventHandle;

namespace CTRPluginFramework
{
    using namespace CTRPluginFrameworkImpl::Services;

    Handle      ProcessImpl::ProcessHandle = 0;
    u32         ProcessImpl::IsPaused = 0;
    u32         ProcessImpl::Status = Running;
    u32         ProcessImpl::ProcessId = 0;
    u64         ProcessImpl::TitleId = 0;

    KThread *   ProcessImpl::MainThread;
    KProcess *  ProcessImpl::KProcessPtr;
    KCodeSet    ProcessImpl::CodeSet;
    u32         ProcessImpl::MainThreadTls;
    MemInfo     ProcessImpl::InvalidRegion = MemInfo{0, 0, 0, 0};
    Mutex       ProcessImpl::MemoryMutex;
    std::vector<MemInfo> ProcessImpl::MemRegions;
    u32         ProcessImpl::exceptionCount = 0;

    void    ProcessImpl::Initialize(void)
    {
        char    kproc[0x100] = {0};
        bool    isNew3DS = System::IsNew3DS();

        // Get current KProcess
        KProcessPtr = KProcess::GetCurrent();
        KProcessPtr->PatchMaxThreads();

        // Copy KProcess data
        Kernel::Memcpy(kproc, KProcessPtr, 0x100);

        if (isNew3DS)
        {
            // Copy KCodeSet
            Kernel::Memcpy(&CodeSet, (void *)*(u32 *)(kproc + 0xB8), sizeof(KCodeSet));

            // Copy process id
            ProcessId = *(u32 *)(kproc + 0xBC);

            // Get main thread
            MainThread = (KThread *)*(u32 *)(kproc + 0xC8);

            // Patch KProcess to allow creating threads on Core2
            KProcessPtr->PatchCore2Access();
        }
        else
        {
            // Copy KCodeSet
            Kernel::Memcpy(&CodeSet, (void *)*(u32 *)(kproc + 0xB0), sizeof(KCodeSet));

            // Copy process id
            ProcessId = *(u32 *)(kproc + 0xB4);

            // Get main thread
            MainThread = (KThread *)*(u32 *)(kproc + 0xC0);
        }

        // Copy title id
        TitleId = CodeSet.titleId;

        // Create handle for this process
        svcOpenProcess(&ProcessHandle, ProcessId);

        UpdateMemRegions();
    }

    extern "C" Handle gspEvent;
    extern "C" bool   IsPaused(void)
    {
        return (ProcessImpl::IsPaused > 0);
    }

    void    ProcessImpl::Pause(bool useFading)
    {
        // Increase pause counter
        ++IsPaused;

        // If game is already paused, nothing to do
        if (IsPaused > 1)
            return;

        Status |= Paused;
        if (Process::OnPauseResume) Process::OnPauseResume(true);

        // Wait for the frame to be paused
        OSDImpl::WaitFramePaused();

        // Acquire screens
        // TODO: error handling
        ScreenImpl::AcquireFromGsp();

        OSDImpl::UpdateScreens();

        // Update memregions
        UpdateMemRegions();
    }

    void    ProcessImpl::Play(bool forced)
    {
        // If game isn't paused, abort
        if (!IsPaused)
            return;

        // Decrease pause counter
        --IsPaused;

        // Force play (reset counter) if requested
        if (forced)
            IsPaused = 0;

        // Resume frame
        if (!IsPaused)
        {
            Status &= ~Paused;
            if (Process::OnPauseResume) Process::OnPauseResume(false);
            ScreenImpl::Top->Release();
            ScreenImpl::Bottom->Release();
            OSDImpl::ResumeFrame();
        }
    }

    bool     ProcessImpl::PatchProcess(u32 addr, u8 *patch, u32 length, u8 *original)
    {
        if (original != nullptr)
        {
            if (!Process::CopyMemory((void *)original, (void *)addr, length))
                goto error;
        }

        if (!Process::CopyMemory((void *)addr, (void *)patch, length))
            goto error;

        return (true);
    error:
        return (false);
    }

    bool     ProcessImpl::PatchFSAccess(bool patch)
    {
        static const std::vector<u32> pattern = { 0x42089800, 0x2000D001 };
        Handle handle;
        s64 info;
        u32 text_addr, text_size, map_addr;

        if(R_SUCCEEDED(svcOpenProcess(&handle, 0)))
        {
            if(R_FAILED(svcGetProcessInfo(&info, handle, 0x10005))) // get start of .text
                return false;
            text_addr = static_cast<u32>(info);

            if(R_FAILED(svcGetProcessInfo(&info, handle, 0x10002))) // get .text size
                return false;
            text_size = static_cast<u32>(info);
            map_addr = GetFreeMemRegion(text_size);

            if(R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, map_addr, handle, text_addr, text_size)))
                return false;

            // Patching fs permisson:
            u32 addr = Utils::Search(map_addr, text_size, pattern);
            if(addr) *(u16*)(addr + 10) = patch ? 0x0200 : 0x4620;


            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, map_addr, text_size);
            svcCloseHandle(handle);

            return true;
        }

        return false;
    }

    void    ProcessImpl::GetHandleTable(KProcessHandleTable& table, std::vector<HandleDescriptor>& handleDescriptors)
    {
        bool    isNew3DS = System::IsNew3DS();

        // Copy KProcessHandleTable
        Kernel::Memcpy(&table, (void *)((u32)KProcessPtr + (isNew3DS ? 0xDC : 0xD4)), sizeof(KProcessHandleTable));

        u32 count = table.handlesCount;

        handleDescriptors.resize(count);
        Kernel::Memcpy(handleDescriptors.data(), table.handleTable, count * sizeof(HandleDescriptor));
    }

    void    ProcessImpl::GetGameThreads(std::vector<KThread *> &threads)
    {
        threads.clear();

        KProcessHandleTable             table;
        std::vector<HandleDescriptor>   handles;

        GetHandleTable(table, handles);

        threads.push_back(MainThread);

        for (HandleDescriptor &handle : handles)
        {
            if (!(handle.obj->GetType() == KType::KThread))
                continue;

            KThread *thread = reinterpret_cast<KThread *>(handle.obj);

            if (!thread->IsPluginThread())
                threads.push_back(thread);
        }
    }

    #define THREADVARS_MAGIC  0x21545624 // !TV$

    // Ensure that only game threads are locked
    static bool    ThreadPredicate(KThread *thread)
    {
        u32 *tls = (u32 *)thread->tls;
        KThread* currentThread = *(KThread**)0xFFFF9000;

        if (currentThread != thread && *tls != THREADVARS_MAGIC)
            return true;
        return false;
    }

    void    ProcessImpl::LockGameThreads(void)
    {
        svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SCHEDULE_THREADS, 1, (u32)ThreadPredicate);
    }

    void    ProcessImpl::UnlockGameThreads(void)
    {
        svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SCHEDULE_THREADS, 0, (u32)ThreadPredicate);
    }

    static bool     IsInRegion(MemInfo &memInfo, u32 addr)
    {
        addr -= memInfo.base_addr;
        return addr < memInfo.size;
    }

    extern "C" u32 __ctru_heap;

    void    ProcessImpl::UpdateMemRegions(bool ignoreLock)
    {
        if (!ignoreLock) MemoryMutex.Lock();

        MemRegions.clear();

        bool    regionPatched  = false;

        for (u32 addr = 0x00100000; addr < 0x40000000; )
        {
            MemInfo     memInfo;
            PageInfo    pageInfo;

            if (R_SUCCEEDED(svcQueryProcessMemory(&memInfo, &pageInfo, ProcessHandle, addr)))
            {
                // If region is FREE, IO, SHARED or LOCKED, skip it
                if (memInfo.state == MEMSTATE_FREE || memInfo.state == MEMSTATE_IO)
                   // || memInfo.state == MEMSTATE_LOCKED || memInfo.state == MEMSTATE_SHARED)
                {
                    addr = memInfo.base_addr + memInfo.size;
                    continue;
                }

                // Same if the memregion is part of CTRPF or NTR
                /*if (memInfo.base_addr == 0x06000000 || memInfo.base_addr == 0x07000000
                    || memInfo.base_addr == 0x01E80000 || IsInRegion(memInfo, __ctru_heap))
                {
                    addr = memInfo.base_addr + memInfo.size;
                    continue;
                }*/

                // Add it to the vector if necessary
                if (memInfo.perm & MEMPERM_READ)
                    MemRegions.push_back(memInfo);

                addr = memInfo.base_addr + memInfo.size;
                continue;
            }

            addr += 0x1000;
        }
        if (!ignoreLock) MemoryMutex.Unlock();
    }

    static bool     IsRegionProhibited(const MemInfo& memInfo)
    {
        // Yes if the memregion is part of CTRPF
        if (memInfo.base_addr == 0x06000000 || memInfo.base_addr == 0x07000000
            || memInfo.base_addr == 0x01E80000)
            return true;

        // Same if the memory is shared
        if (memInfo.state == MEMSTATE_SHARED)
            return true;

        return false;
    }

    bool        ProcessImpl::IsValidAddress(const u32 address)
    {
        Lock lock(MemoryMutex);

        for (MemInfo &memInfo : MemRegions)
            if (IsInRegion(memInfo, address))
                return true;

        return false;
    }

    u32        ProcessImpl::GetPAFromVA(const u32 address)
    {
        u32 pa = 0;

        svcControlProcess(ProcessImpl::ProcessHandle, PROCESSOP_GET_PA_FROM_VA, (u32)&pa, address);

        return pa;
    }

    MemInfo     ProcessImpl::GetMemRegion(u32 address)
    {
        Lock lock(MemoryMutex);

        for (MemInfo &memInfo : MemRegions)
            if (IsInRegion(memInfo, address))
                return memInfo;

        // Not found return an empty region
        return ProcessImpl::InvalidRegion;
    }

    MemInfo     ProcessImpl::GetNextRegion(const MemInfo &region)
    {
        Lock lock(MemoryMutex);

        for (MemInfo &memInfo : MemRegions)
            if (memInfo > region && !IsRegionProhibited(memInfo))
                return memInfo;

        return region;
    }

    MemInfo     ProcessImpl::GetPreviousRegion(const MemInfo &region)
    {
        Lock lock(MemoryMutex);

        MemInfo *prev = nullptr;

        for (MemInfo &memInfo : MemRegions)
        {
            if (memInfo >= region && !IsRegionProhibited(memInfo))
                return prev != nullptr ? *prev : memInfo;

            prev = &memInfo;
        }

        return region;
    }

    void ProcessImpl::EnableExceptionHandlers()
    {
        if (MainThreadTls)
        {
            *(u32*)(MainThreadTls + 0x40) = (u32)ProcessImpl::ExceptionHandler;
            *(u32*)(MainThreadTls + 0x44) = 1;//(u32)&stack[0x1000];
            *(u32*)(MainThreadTls + 0x48) = 1;//(u32)&exceptionData;
        }
    }

    void ProcessImpl::DisableExceptionHandlers()
    {
        if (MainThreadTls)
            *(u32*)(MainThreadTls + 0x40) = 0;
    }

    void    ProcessImpl::ReturnFromException(CpuRegisters* regs) {
#ifndef _MSC_VER
        __asm__ __volatile__(
            "ldr sp,    [r0,#0x34]  @sp \n"
            "ldr r1,    [r0, #0x3c] @pc \n"
            "str r1,    [sp, #-4]!      \n"
            "ldr r1,    [r0, #0x38] @lr \n"
            "str r1,    [sp, #-4]!      \n"
            "mov r2,    #0x30           \n"

            "_store_reg_loop:           \n"
            "ldr r1,    [r0, r2]        \n"
            "str r1,    [sp, #-4]!      \n"
            "sub r2,    r2, #4          \n"
            "cmp r2,    #0              \n"
            "bge        _store_reg_loop \n"

            "ldr r1,    [r0, #0x40]     \n"
            "msr cpsr,  r1              \n"
            "ldmfd sp!, {r0-r12, lr, pc}\n"
        );
#endif
    }

    void    ProcessImpl::ExceptionHandler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
        // Default exception handler, if the user didn't set an custom exception handler or an exception happened in the user callback
        if (AtomicPostIncrement(&exceptionCount) || !Process::exceptionCallback) {
            DisableExceptionHandlers();
            ReturnFromException(regs);
        }
        // Lock game threads
        LockGameThreads();

        // Resume interrupt reciever and acquire screens
        // NOTE: NEEDS TO BE DISABLED IF THIS FUNCTION IS MADE TO RETURN EXECUTION
        GSP::ResumeInterruptReceiver();
        if (!ScreenImpl::AcquireFromGsp())
            // Update OSD screens
            OSDImpl::UpdateScreens();

        // Update memregions, this layout is used by internal checks
        UpdateMemRegions(true);

        Process::ExceptionCallbackState ret = Process::EXCB_LOOP;

        while (ret == Process::EXCB_LOOP)
            ret = Process::exceptionCallback(excep, regs);

        switch (ret)
        {
        case Process::EXCB_REBOOT:
            System::Reboot();
            break;
        case Process::EXCB_RETURN_HOME:
            Process::ReturnToHomeMenu();
            break;
        default:
            // Rethrow the exception to the default exceptions handlers
            DisableExceptionHandlers();
            ReturnFromException(regs);
            break;
        }
    }
}
