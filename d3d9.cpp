#include <stdio.h>
#include <Windows.h>
#include "detours.h"

FARPROC j_Direct3DCreate9;
extern "C" __declspec(dllexport) void Direct3DCreate9() { j_Direct3DCreate9(); }

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
	lstrcpy(lpTimeZoneInformation->StandardName, L"ìåãû");
	return mResult;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		WCHAR wcsPath[MAX_PATH];
		GetSystemDirectory(wcsPath, MAX_PATH);
		lstrcat(wcsPath, L"\\d3d9.dll");
		HMODULE hHijack = LoadLibrary(wcsPath);
		if (hHijack == NULL) {
			MessageBox(NULL, L"ñ≥ñ@ç⁄ì¸ d3d9.dll íˆéÆå…", L"", MB_ICONERROR);
			return FALSE;
		}
		j_Direct3DCreate9 = GetProcAddress(hHijack, "Direct3DCreate9");

		DetourTransactionBegin();
		DetourAttach((PVOID*)&o_VerQueryValueW, h_VerQueryValueW);
		DetourAttach((PVOID*)&o_GetLocaleInfoW, h_GetLocaleInfoW);
		DetourAttach((PVOID*)&o_GetTimeZoneInformation, h_GetTimeZoneInformation);
		DetourTransactionCommit();
	}
	return TRUE;
}