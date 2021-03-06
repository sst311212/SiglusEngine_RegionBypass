#include <stdio.h>
#include <Windows.h>
#include "detours.h"

FARPROC j_DirectSoundCreate;
void DirectSoundCreate() { j_DirectSoundCreate(); }

FARPROC j_Direct3DCreate9;
void Direct3DCreate9() { j_Direct3DCreate9(); }

typedef BOOL(APIENTRY *t_VerQueryValueA)(LPCVOID, LPCSTR, LPVOID*, PUINT);
t_VerQueryValueA o_VerQueryValueA = VerQueryValueA;

BOOL APIENTRY h_VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)
{
	auto mResult = o_VerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
	if (lstrcmpiA(lpSubBlock, "\\VarFileInfo\\Translation") == 0) {
		struct LANGCODEPAGE { WORD wLang; WORD wCode; };
		((LANGCODEPAGE*)*lplpBuffer)->wLang = 0x411;
		((LANGCODEPAGE*)*lplpBuffer)->wCode = 0x411;
	}
	return mResult;
}

typedef BOOL(APIENTRY *t_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
t_VerQueryValueW o_VerQueryValueW = VerQueryValueW;

BOOL APIENTRY h_VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)
{
	auto mResult = o_VerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);
	if (lstrcmpi(lpSubBlock, L"\\VarFileInfo\\Translation") == 0) {
		struct LANGCODEPAGE { WORD wLang; WORD wCode; };
		((LANGCODEPAGE*)*lplpBuffer)->wLang = 0x411;
		((LANGCODEPAGE*)*lplpBuffer)->wCode = 0x411;
	}
	return mResult;
}

typedef int(WINAPI *t_GetLocaleInfoA)(LCID, LCTYPE, LPSTR, int);
t_GetLocaleInfoA o_GetLocaleInfoA = GetLocaleInfoA;

int WINAPI h_GetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
	auto mResult = o_GetLocaleInfoA(Locale, LCType, lpLCData, cchData);
	if (Locale == LOCALE_SYSTEM_DEFAULT)
		lstrcpyA(lpLCData, "0411");
	return mResult;
}

typedef int(WINAPI *t_GetLocaleInfoW)(LCID, LCTYPE, LPWSTR, int);
t_GetLocaleInfoW o_GetLocaleInfoW = GetLocaleInfoW;

int WINAPI h_GetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	auto mResult = o_GetLocaleInfoW(Locale, LCType, lpLCData, cchData);
	if (Locale == LOCALE_SYSTEM_DEFAULT)
		lstrcpy(lpLCData, L"0411");
	return mResult;
}

typedef DWORD(WINAPI *t_GetTimeZoneInformation)(LPTIME_ZONE_INFORMATION);
t_GetTimeZoneInformation o_GetTimeZoneInformation = GetTimeZoneInformation;

DWORD WINAPI h_GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
	auto mResult = o_GetTimeZoneInformation(lpTimeZoneInformation);
	lstrcpy(lpTimeZoneInformation->StandardName, L"東京");
	return mResult;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		WCHAR lpSysPath[MAX_PATH], lpDllPath[MAX_PATH];
		GetSystemDirectory(lpSysPath, MAX_PATH);
		swprintf_s(lpDllPath, L"%s\\d3d9.dll", lpSysPath);
		HMODULE hHijack = LoadLibrary(lpDllPath);
		if (hHijack != NULL)
			j_Direct3DCreate9 = GetProcAddress(hHijack, "Direct3DCreate9");
		swprintf_s(lpDllPath, L"%s\\dsound.dll", lpSysPath);
		hHijack = LoadLibrary(lpDllPath);
		if (hHijack != NULL)
			j_DirectSoundCreate = GetProcAddress(hHijack, "DirectSoundCreate");

		DetourTransactionBegin();
		DetourAttach((PVOID*)&o_VerQueryValueA, h_VerQueryValueA);
		DetourAttach((PVOID*)&o_VerQueryValueW, h_VerQueryValueW);
		DetourAttach((PVOID*)&o_GetLocaleInfoA, h_GetLocaleInfoA);
		DetourAttach((PVOID*)&o_GetLocaleInfoW, h_GetLocaleInfoW);
		DetourAttach((PVOID*)&o_GetTimeZoneInformation, h_GetTimeZoneInformation);
		DetourTransactionCommit();
	}
	return TRUE;
}