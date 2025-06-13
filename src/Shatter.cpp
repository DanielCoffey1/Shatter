#include <windows.h>
#include <shellapi.h>
#include <psapi.h> // <-- Required for GetModuleFileNameEx
#include <string>
#include "resource.h"

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_TOGGLE 1002
#define ID_TRAY_AUTOSTART 1003

HINSTANCE hInst;
NOTIFYICONDATA nid = {};
HMENU hTrayMenu = NULL;
bool enableXKill = true;
bool IsAutostartEnabled() {
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return false;

	WCHAR path[MAX_PATH];
	DWORD size = sizeof(path);
	LONG result = RegQueryValueEx(hKey, L"Shatter", NULL, NULL, (LPBYTE)path, &size);
	RegCloseKey(hKey);
	return result == ERROR_SUCCESS;
}

void ToggleAutostart() {
	HKEY hKey;
	const wchar_t* key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) return;

	if (IsAutostartEnabled()) {
		RegDeleteValue(hKey, L"Shatter");
	}
	else {
		WCHAR exePath[MAX_PATH];
		GetModuleFileName(NULL, exePath, MAX_PATH);

		// Wrap path in quotes
		std::wstring quotedPath = L"\"" + std::wstring(exePath) + L"\"";

		RegSetValueExW(hKey, L"Shatter", 0, REG_SZ,
			(const BYTE*)quotedPath.c_str(),
			static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t)));
	}

	RegCloseKey(hKey);
}



bool IsBlacklisted(const std::wstring& exeName) {
	return exeName == L"explorer.exe";
}

void KillForegroundWindow() {
	HWND hwnd = GetForegroundWindow();
	if (!hwnd) return;

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pid);

	WCHAR path[MAX_PATH];
	GetModuleFileNameEx(hProcess, NULL, path, MAX_PATH);
	std::wstring exeName = std::wstring(path).substr(std::wstring(path).find_last_of(L"\\") + 1);

	if (!IsBlacklisted(exeName)) {
		TerminateProcess(hProcess, 0);
	}

	CloseHandle(hProcess);
}

void ToggleEnableXKill() {
	enableXKill = !enableXKill;
	CheckMenuItem(hTrayMenu, ID_TRAY_TOGGLE, MF_BYCOMMAND | (enableXKill ? MF_CHECKED : MF_UNCHECKED));
}

void InitTray(HWND hwnd) {
	hTrayMenu = CreatePopupMenu();
	AppendMenu(hTrayMenu, MF_STRING | (enableXKill ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_TOGGLE, L"Enable XKill");
	AppendMenu(hTrayMenu, MF_STRING | (IsAutostartEnabled() ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_AUTOSTART, L"Start with Windows");
	AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SHATTER_ICON));
	wcscpy_s(nid.szTip, L"Shatter");
	Shell_NotifyIcon(NIM_ADD, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_HOTKEY:
		if (enableXKill && wParam == 1) {
			KillForegroundWindow();
		}
		break;
	case WM_TRAYICON:
		if (lParam == WM_RBUTTONUP) {
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_TRAY_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
			break;
		case ID_TRAY_AUTOSTART:
			ToggleAutostart();
			CheckMenuItem(hTrayMenu, ID_TRAY_AUTOSTART, MF_BYCOMMAND | (IsAutostartEnabled() ? MF_CHECKED : MF_UNCHECKED));
			break;
		case ID_TRAY_TOGGLE:
			ToggleEnableXKill();
			break;
		}
		break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	hInst = hInstance;

	const wchar_t CLASS_NAME[] = L"ShatterWndClass";
	WNDCLASS wc = { };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Shatter", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hwnd) return 1;

	RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_ALT, VK_F4);
	InitTray(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
