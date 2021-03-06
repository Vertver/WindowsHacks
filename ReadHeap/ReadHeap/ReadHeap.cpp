/*********************************************************
* Copyright (C) VERTVER, 2018. All rights reserved.
* NTSamples - Native API examples
* MIT-License
**********************************************************
* Module Name: Reading process heaps
*********************************************************/

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <subauth.h>

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID;

typedef struct _PEB
{
	UCHAR InheritedAddressSpace;
	UCHAR ReadImageFileExecOptions;
	UCHAR BeingDebugged;
	UCHAR BitField;
	ULONG ImageUsesLargePages : 1;
	ULONG IsProtectedProcess : 1;
	ULONG IsLegacyProcess : 1;
	ULONG IsImageDynamicallyRelocated : 1;
	ULONG SpareBits : 4;
	PVOID Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PRTL_CRITICAL_SECTION FastPebLock;
	PVOID AtlThunkSListPtr;
	PVOID IFEOKey;
	ULONG CrossProcessFlags;
	ULONG ProcessInJob : 1;
	ULONG ProcessInitializing : 1;
	ULONG ReservedBits0 : 30;
	union
	{
		PVOID KernelCallbackTable;
		PVOID UserSharedInfoPtr;
	};
	ULONG SystemReserved[1];
	ULONG SpareUlong;
	DWORD FreeList[4];
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID HotpatchInformation;
	VOID** ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	LARGE_INTEGER CriticalSectionTimeout;
	ULONG HeapSegmentReserve;
	ULONG HeapSegmentCommit;
	ULONG HeapDeCommitTotalFreeThreshold;
	ULONG HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	VOID** ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	ULONG GdiDCAttributeList;
	PRTL_CRITICAL_SECTION LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	WORD OSBuildNumber;
	WORD OSCSDVersion;
	ULONG OSPlatformId;
	ULONG ImageSubsystem;
	ULONG ImageSubsystemMajorVersion;
	ULONG ImageSubsystemMinorVersion;
	ULONG ImageProcessAffinityMask;
	ULONG GdiHandleBuffer[34];
	PVOID PostProcessInitRoutine;
	PVOID TlsExpansionBitmap;
	ULONG TlsExpansionBitmapBits[32];
	ULONG SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	PVOID pShimData;
	PVOID AppCompatInfo;
	UNICODE_STRING CSDVersion;
	LPVOID ActivationContextData;
	LPVOID ProcessAssemblyStorageMap;
	LPVOID SystemDefaultActivationContextData;
	LPVOID SystemAssemblyStorageMap;
	ULONG MinimumStackCommit;
	LPVOID FlsCallback;
	LIST_ENTRY FlsListHead;
	PVOID FlsBitmap;
	ULONG FlsBitmapBits[4];
	ULONG FlsHighIndex;
	PVOID WerRegistrationData;
	PVOID WerShipAssertPtr;
} PEB, *PPEB;

typedef struct PBI {
	PVOID Reserved1;
	PVOID PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} ___PROCESS_BASIC_INFORMATION;

typedef enum __PROCESSINFOCLASS {
	ProcessBasicInformation = 0,	// <- use this
	ProcessDebugPort = 7,
	ProcessWow64Information = 26,
	ProcessImageFileName = 27,
	ProcessBreakOnTermination = 29
} ___PROCESSINFOCLASS;

typedef LONG(NTAPI *NTQUERYINFORMATIONPROCESS)(HANDLE, __PROCESSINFOCLASS, PVOID, ULONG, PULONG);
typedef LONG(NTAPI *NTREADVIRTUALMEMORY)(HANDLE, PVOID, PVOID, ULONG, PULONG);

int
main()
{
	DWORD dwProcessId = NULL;

	// get process id
	printf("Write process ID \n");
	scanf_s("%u", &dwProcessId);

	// open process by id
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (!hProcess) { ExitProcess(DWORD(-2)); }

	// get adressees to functions
	NTQUERYINFORMATIONPROCESS pNtQueryInformationProcess = (NTQUERYINFORMATIONPROCESS)GetProcAddress(GetModuleHandleA("ntdll"), "NtQueryInformationProcess");
	NTREADVIRTUALMEMORY pNtReadVirtualMemory = (NTREADVIRTUALMEMORY)GetProcAddress(GetModuleHandleA("ntdll"), "NtReadVirtualMemory");
	
	___PROCESS_BASIC_INFORMATION processInformation = { NULL };
	DWORD cbLength = NULL;

	// get info about process
	if (!pNtQueryInformationProcess) { ExitProcess(DWORD(-2)); }
	pNtQueryInformationProcess(hProcess, ProcessBasicInformation, &processInformation, sizeof(___PROCESS_BASIC_INFORMATION), &cbLength);
	printf("NtQueryInformationProcess() succeeded \n");

	PEB pebStruct = { NULL };
	BYTE byteProcess[0x010000] = { NULL };
	BYTE sharedMemory[0x010000] = { NULL };

	// read PEB from pointer
	pNtReadVirtualMemory(hProcess, processInformation.PebBaseAddress, &pebStruct, sizeof(PEB), &cbLength);
	pNtReadVirtualMemory(hProcess, pebStruct.ProcessHeap, byteProcess, 0x010000, &cbLength);
	pNtReadVirtualMemory(hProcess, pebStruct.ReadOnlySharedMemoryBase, sharedMemory, 0x010000, &cbLength);
	printf("Memory readed \n");

	CHAR szPath[MAX_PATH] = { NULL };
	if (GetCurrentDirectoryA(MAX_PATH, szPath))
	{
		/*
		*	This is old project, when I'm using std::string (but I don't like that) 
		*	so please, don't report me this. I know about that case.
		*/

		// get path to write our file (default debug directory - solution dir)
		std::string szFullPath = szPath;
		std::string szHeap = szFullPath + "ProcessHeap.txt";
		std::string szShared = szFullPath + "SharedMemory (RO).txt";

		// write heap to .txt file
		try
		{
			HANDLE hHeapFile = CreateFileA(szHeap.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hHeapFile) { WriteFile(hHeapFile, byteProcess, sizeof(byteProcess), &cbLength, NULL); }
			CloseHandle(hHeapFile);

			// write read-only shared memory to .txt file
			HANDLE hSharedFile = CreateFileA(szShared.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hSharedFile) { WriteFile(hSharedFile, sharedMemory, sizeof(sharedMemory), &cbLength, NULL); }
			CloseHandle(hSharedFile);
		}
		catch (const std::exception& exc)
		{
			// in 99% cases - it's handle closing exception, so ignore this
			printf("(%s). \n", exc.what());
		}

		// ready to exit
		printf("Result in working directory (press any key to continue)\n");
		_getwch();
	}

	ExitProcess(0);
	return 0;
}
