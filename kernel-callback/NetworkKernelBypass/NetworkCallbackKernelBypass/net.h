#pragma once
#define ASSERT_SZ( x, y ) static_assert(sizeof(x) == y, "incorrect size for " #x);

// begin usermode defs
#include <Windows.h>
#include <winternl.h>

// Define the structure based on the information provided
typedef struct WFP_STRUCT {
	DWORD firstDword;  // First DWORD (4 bytes)
	DWORD secondDword;     // Padding to align to 8 bytes (QWORD alignment)
	DWORD64 padding2; // Second QWORD (8 bytes)
	DWORD64 classifyFn;
	DWORD64 notifyFn;
	DWORD64 deleteFn;
	DWORD64 classifyFn2;
};