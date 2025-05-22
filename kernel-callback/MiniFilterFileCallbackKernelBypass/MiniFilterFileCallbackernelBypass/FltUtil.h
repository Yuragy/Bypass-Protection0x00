#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "FltDef.h"
#include "MemHandler.h"
#include <map>

// I can hear the OSR replies now... 
#define FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD 0x58
#define FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_LIST 0x68
#define FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_COUNT 0x78

#define FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD 0x48
#define FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT 0x78
#define FILTER_RESOUCE_LISTHEAD_OFFSET_FILTER_LISTHEAD 0x68

#define FILTER_OFFSET_NAME 0x40
#define FILTER_OFFSET_FRAME 0x38
#define FILTER_OFFSET_OPERATIONS 0x1b0
#define FILTER_OFFSET_INSTANCELIST 0x68

#define FILTER_INSTANCELIST_OFFSET_INSTANCES_COUNT 0x78
#define FILTER_INSTANCELIST_OFFSET_INSTANCES_LIST 0x68

#define FRAME_OFFSET_VOLUME_LIST 0xc8
#define VOLUME_LIST_OFFSET_COUNT 0x78
#define VOLUME_LIST_OFFSET_LIST 0x68

#define VOLUME_OFFSET_DEVICE_NAME 0x70
#define VOLUME_OFFSET_CALLBACK_TBL 0x140

#define CALLBACK_NODE_OFFSET_PREOP 0x18
#define CALLBACK_NODE_OFFSET_POSTOP 0x20

#define UNISTR_OFFSET_LEN 0
#define UNISTR_OFFSET_BUF 8

//FLTMGR!FltEnumerateFilters + 0x81:
//fffff800`350c90e1 e87a59316e      call    nt!ExInitializeFastOwnerEntry(fffff800`a33dea60)
//fffff800`350c90e6 4c8b157310fdff  mov     r10, qword ptr[FLTMGR!_imp_KeEnterCriticalRegion(fffff800`3509a160)]
//fffff800`350c90ed e8fe23326e      call    nt!KeEnterCriticalRegion(fffff800`a33eb4f0)
//fffff800`350c90f2 41b001          mov     r8b, 1
//fffff800`350c90f5 488d942480000000 lea     rdx, [rsp + 80h]
//fffff800`350c90fd 488d0d9476fcff  lea     rcx, [FLTMGR!FltGlobals + 0x58 (fffff800`35090798)]
//fffff800`350c9104 4c8b154d10fdff  mov     r10, qword ptr[FLTMGR!_imp_ExAcquireFastResourceShared(fffff800`3509a158)]
//fffff800`350c910b e820da146e      call    nt!ExAcquireFastResourceShared(fffff800`a3216b30)
const uint8_t patternFltGlobals[] = { 0x48, 0x8d, 0x0d, 0x58 };

class FltManager
{
public:
	FltManager(MemHandler* objMemHandler);
	~FltManager();
	PVOID lpNtosBase = { 0 };
	PVOID lpFltMgrBase = { 0 };
	PVOID lpFltGlobals = { 0 };
	PVOID lpFltFrameList = { 0 };
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchCallbackMap;
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchLinksMap;
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchCFGMap;
	PVOID GetFilterByName(const wchar_t* strFilterName);
	PVOID GetFrameForFilter(LPVOID lpFilter);
	std::vector<FLT_OPERATION_REGISTRATION> GetOperationsForFilter(PVOID lpFilter);
	BOOL Restore();
	std::unordered_map<wchar_t*, PVOID> EnumFrameVolumes(LPVOID lpFrame);
	DWORD GetFrameCount();
	BOOL UnLinksForVolumesAndCallbacks(
		std::vector<FLT_OPERATION_REGISTRATION> vecTargetOperations,
		std::unordered_map<wchar_t*, PVOID> mapTargetVolumes,
		UCHAR ToRemove
	);

private:
	ULONG ulNumFrames;
	PVOID ResolveDriverBase(const wchar_t* strDriverName);
	PVOID ResolveFltmgrGlobals(LPVOID lpkFltMgrBase);
	MemHandler* objMemHandler;

};

static std::unordered_map<BYTE, const char*> g_IrpMjMap{
	{0, "IRP_MJ_CREATE"},
	{1, "IRP_MJ_CREATE_NAMED_PIPE"},
	{2, "IRP_MJ_CLOSE"},
	{3, "IRP_MJ_READ"},
	{4, "IRP_MJ_WRITE"},
	{5, "IRP_MJ_QUERY_INFORMATION"},
	{6, "IRP_MJ_SET_INFORMATION"},
	{7, "IRP_MJ_QUERY_EA"},
	{8, "IRP_MJ_SET_EA"},
	{9, "IRP_MJ_FLUSH_BUFFERS"},
	{0xa, "IRP_MJ_QUERY_VOLUME_INFORMATION"},
	{0xb, "IRP_MJ_SET_VOLUME_INFORMATION"},
	{0xc, "IRP_MJ_DIRECTORY_CONTROL"},
	{0xd, "IRP_MJ_FILE_SYSTEM_CONTROL"},
	{0xe, "IRP_MJ_DEVICE_CONTROL"},
	{0xf, "IRP_MJ_INTERNAL_DEVICE_CONTROL"},
	{0x10, "IRP_MJ_SHUTDOWN"},
	{0x11, "IRP_MJ_LOCK_CONTROL"},
	{0x12, "IRP_MJ_CLEANUP"},
	{0x13, "IRP_MJ_CREATE_MAILSLOT"},
	{0x14, "IRP_MJ_QUERY_SECURITY"},
	{0x15, "IRP_MJ_SET_SECURITY"},
	{0x16, "IRP_MJ_POWER"},
	{0x17, "IRP_MJ_SYSTEM_CONTROL"},
	{0x18, "IRP_MJ_DEVICE_CHANGE"},
	{0x19, "IRP_MJ_QUERY_QUOTA"},
	{0x1a, "IRP_MJ_SET_QUOTA"},
	{0x1b, "IRP_MJ_PNP"},
	{0x1b, "IRP_MJ_PNP_POWER"},
	{0x1b, "IRP_MJ_MAXIMUM_FUNCTION"},
	{((UCHAR)-1), "IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION"},
	{((UCHAR)-2), "IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION"},
	{((UCHAR)-3), "IRP_MJ_ACQUIRE_FOR_MOD_WRITE"},
	{((UCHAR)-4), "IRP_MJ_RELEASE_FOR_MOD_WRITE"},
	{((UCHAR)-5), "IRP_MJ_ACQUIRE_FOR_CC_FLUSH"},
	{((UCHAR)-6), "IRP_MJ_RELEASE_FOR_CC_FLUSH"},
	{((UCHAR)-7), "IRP_MJ_QUERY_OPEN"},
	{((UCHAR)-13), "IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE"},
	{((UCHAR)-14), "IRP_MJ_NETWORK_QUERY_OPEN"},
	{((UCHAR)-15), "IRP_MJ_MDL_READ"},
	{((UCHAR)-16), "IRP_MJ_MDL_READ_COMPLETE"},
	{((UCHAR)-17), "IRP_MJ_PREPARE_MDL_WRITE"},
	{((UCHAR)-18), "IRP_MJ_MDL_WRITE_COMPLETE"},
	{((UCHAR)-19), "IRP_MJ_VOLUME_MOUNT"},
	{((UCHAR)-20), "IRP_MJ_VOLUME_DISMOUNT"}
};
