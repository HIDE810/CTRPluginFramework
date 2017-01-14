#include "CTRPluginFramework.hpp"
#include "arm11kCommands.h"
#include "3DS.h"
#include <cstdio>
#include <cstring>

extern 		Handle gspThreadEventHandle;

namespace CTRPluginFramework
{
	u32         Process::_processID = 0;
	u64         Process::_titleID = 0;
	char        Process::_processName[8] = {0};
	u32         Process::_kProcess = 0;
	u32			Process::_kProcessState = 0;
	KCodeSet    Process::_kCodeSet = {0};
	Handle 		Process::_processHandle = 0;
	Handle 		Process::_mainThreadHandle = 0;

	//u32 		Process::_finishedStateDMA = 0;
	//u32         *Process::_kProcessHandleTable = nullptr;


	void    Process::Initialize(Handle threadHandle, bool isNew3DS)
	{
		char    kproc[0x100] = {0};

		// Get current KProcess
		_kProcess = (u32)arm11kGetCurrentKProcess();
		
		// Copy KProcess data
		arm11kMemcpy((u32)&kproc, _kProcess, 0x100);
		if (isNew3DS)
		{
			// Copy KCodeSet
			arm11kMemcpy((u32)&_kCodeSet, *(u32 *)(kproc + 0xB8), sizeof(KCodeSet));          

			// Copy process id
			_processID = *(u32 *)(kproc + 0xBC);
			_kProcessState = _kProcess + 0x88;
		}
		else
		{
			// Copy KCodeSet
			arm11kMemcpy((u32)&_kCodeSet, *(u32 *)(kproc + 0xB0), sizeof(KCodeSet));          

			// Copy process id
			_processID = *(u32 *)(kproc + 0xB4);
			_kProcessState = _kProcess + 0x80;
		}

		// Copy process name
		for (int i = 0; i < 8; i++)
				_processName[i] = _kCodeSet.processName[i];

		// Copy title id
		_titleID = _kCodeSet.titleId;
		// Create handle for this process
		svcOpenProcess(&_processHandle, _processID);
		// Set plugin's main thread handle
		_mainThreadHandle = threadHandle;
	}

	Handle 	Process::GetHandle(void)
	{
		return (_processHandle);
	}

	u32     Process::GetProcessID(void)
	{
		return (_processID);
	}

	void     Process::GetProcessID(char *output)
	{
		if (!output)
			return;
		sprintf(output, "%02X", _processID);
	}

	u64     Process::GetTitleID(void)
	{
		return (_titleID);
	}

	void     Process::GetTitleID(char *output)
	{
		if (!output)
			return;
		sprintf(output, "%016llX", _titleID);
	}

	void    Process::GetName(char *output)
	{
		if (output != nullptr)
			for (int i = 0; i < 8; i++)
				output[i] = _processName[i];
	}

	u8 		Process::GetProcessState(void)
	{
		return (arm11kGetKProcessState(_kProcessState));
	}

	bool 	Process::Patch(u32 	addr, u8 *patch, u32 length, u8 *original)
	{
		return (PatchProcess(addr, patch, length, original));
	}

	void 	Process::Pause(void)
	{
		svcSetThreadPriority(gspThreadEventHandle, 0x19);
		svcSetThreadPriority(_mainThreadHandle, 0x20);
	}

	void 	Process::Play(void)
	{
		svcSetThreadPriority(gspThreadEventHandle, 0x31);
		svcSetThreadPriority(_mainThreadHandle, 0x3F);
	}

    bool     Process::ProtectMemory(u32 addr, u32 size, int perm)
    {
    	if (R_FAILED(svcControlProcessMemory(_processHandle, addr, addr, size, 6, perm)))
        	return (false);
        return (true);
    }

    bool     Process::ProtectRegion(u32 addr, int perm)
    {
    	MemInfo 	minfo;
    	PageInfo 	pinfo;

    	if (R_FAILED(svcQueryProcessMemory(&minfo, &pinfo, _processHandle, addr))) goto error;
    	if (minfo.state == MEMSTATE_FREE) goto error;
    	if (addr < minfo.base_addr || addr > minfo.base_addr + minfo.size) goto error;

    	return (ProtectMemory(minfo.base_addr, minfo.size, perm));
    error:
        return (false);
    }

    bool     Process::PatchProcess(u32 addr, u8 *patch, u32 length, u8 *original)
    {
        if (!(ProtectMemory(((addr / 0x1000) * 0x1000), 0x1000))) goto error;
 
 		if (original != nullptr)
 		{
 			if (!CopyMemory((void *)original, (void *)addr, length))
 				goto error;
 		}

 		if (!CopyMemory((void *)addr, (void *)patch, length))
 			goto error;
        return (true);
    error:
        return (false);
    }

    bool     Process::CopyMemory(void *dst, void *src, u32 size)
    {
        if (R_FAILED(svcFlushProcessDataCache(_processHandle, src, size))) goto error;
        if (R_FAILED(svcFlushProcessDataCache(_processHandle, dst, size))) goto error;
        std::memcpy(dst, src, size);
        svcInvalidateProcessDataCache(_processHandle, dst, size);
        return (true);
    error:
        return (false);
    }
}