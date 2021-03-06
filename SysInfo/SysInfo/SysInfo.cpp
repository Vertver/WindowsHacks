/*********************************************************
* Copyright (C) VERTVER, 2019. All rights reserved.
* WindowsHacks - project with hacks for Windows
* MIT-License
**********************************************************
* Module Name: Using NtQuerySystemInformation()
*********************************************************/

#include "pch.h"
#include <iostream>
#include <windows.h>

// get here our defines
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

// define our pointer to function (NtQuerySystemInformation() is deprecated now)
typedef HRESULT(WINAPI *NTQUERYSYSTEMINFORMATION)(UINT, PVOID, ULONG, PULONG);

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER DpcTime;
	LARGE_INTEGER InterruptTime;
	ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

// entry-point
int
main()
{
	// get adress to our func to use
	NTQUERYSYSTEMINFORMATION pNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(
		GetModuleHandleA("ntdll.dll"),
		"NtQuerySystemInformation"
	);

	// if we can't find  NtQuerySystemInformation() in ntdll.dll - exit 
	if (!pNtQuerySystemInformation) { ExitProcess(0xFFFFFFFF); }

	printf("Entry point \n");

	// get system info for get cpu threads
	SYSTEM_INFO sysInfo = { NULL };
	GetSystemInfo(&sysInfo);
	PFLOAT fUsage = (PFLOAT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FLOAT) * sysInfo.dwNumberOfProcessors);

	auto perfomanceInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * sysInfo.dwNumberOfProcessors);
	PLARGE_INTEGER m_idleTime = (PLARGE_INTEGER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LARGE_INTEGER) * sysInfo.dwNumberOfProcessors);
	DWORD m_dwCount = 0;

	while (TRUE)
	{
		// get perfomance info by NtQuerySystemInformation()
		if (NT_SUCCESS(pNtQuerySystemInformation(8, perfomanceInfo, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)sysInfo.dwNumberOfProcessors, NULL)))
		{
			printf("NtQuerySystemInformation return NT_SUCCESS \n");
		}

		DWORD dwTickCount = GetTickCount();
		if (!m_dwCount) m_dwCount = dwTickCount;

		// get cycle with counting deltas of CPU load
		for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		{
			FLOAT fTemp = 0.0f;
			PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuPerformanceInfo = &perfomanceInfo[i];
			cpuPerformanceInfo->KernelTime.QuadPart -= cpuPerformanceInfo->IdleTime.QuadPart;

			// to get info about load per CPU, we need to get by this
			fUsage[i] = 100.0f - 0.01f * (cpuPerformanceInfo->IdleTime.QuadPart - m_idleTime[i].QuadPart) / ((dwTickCount - m_dwCount));
			fTemp = fUsage[i];
			if (fUsage[i] < 0.0f) { fUsage[i] = 0.0f; }
			if (fUsage[i] > 100.0f) { fUsage[i] = 100.0f; }

			m_idleTime[i] = cpuPerformanceInfo->IdleTime;
		}	

		for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		{
			printf("%i-CPU: %f\n", i, fUsage[i]);
		}
		
		m_dwCount = dwTickCount;
		Sleep(250);
	}
}
