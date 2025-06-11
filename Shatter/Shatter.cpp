#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

#define HOTKEY_KILL     1
#define HOTKEY_XKILL    2
#define ID_TRAY_EXIT    1001
#define WM_TRAYICON     (WM_USER + 1)

HINSTANCE hInst;
HHOOK mouseHook;
bool inXKillMode = false;
HCURSOR killCursor;
NOTIFYICONDATA nid = {};
HWND hWndMain;

void KillForegroundWindow() {
    HWND hwnd = GetForegroundWindow();
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProc) {
        TerminateProcess(hProc, 1);
        CloseHandle(hProc);
    }
}

void StartXKillMode() {
    inXKillMode = true;
    killCursor = LoadCursorFromFile(_T("shatter_kill.cur"));
    if (!killCursor) killCursor = LoadCursor(NULL, IDC_CROSS);
    SetCursor(killCursor);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (nCode >= 0 && inXKillMode) {
            if (wParam == WM_LBUTTONDOWN) {
                POINT pt;
                GetCursorPos(&pt);
                HWND hwnd = WindowFromPoint(pt);
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 1);
                    CloseHandle(hProc);
                }
                inXKillMode = false;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                UnhookWindowsHookEx(mouseHook);
            }
            else if (wParam == WM_RBUTTONDOWN) {
                inXKillMode = false;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                UnhookWindowsHookEx(mouseHook);
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
        }, NULL, 0);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT, _T("Exit Shatter"));
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        if (wParam == HOTKEY_KILL) KillForegroundWindow();
        else if (wParam == HOTKEY_XKILL) StartXKillMode();
    }
    else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
    }
    else if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt);
        }
    }
    else if (msg == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    hInst = hInstance;

    const TCHAR CLASS_NAME[] = _T("ShatterTrayWindow");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hWndMain = CreateWindow(CLASS_NAME, _T("Shatter"), 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    // Register hotkeys
    RegisterHotKey(hWndMain, HOTKEY_KILL, MOD_CONTROL | MOD_ALT, VK_F4);
    RegisterHotKey(hWndMain, HOTKEY_XKILL, MOD_WIN, VK_F4);

    // Tray icon setup
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWndMain;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Optional: Replace with custom icon
    _tcscpy_s(nid.szTip, _T("Shatter is running"));
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    UnregisterHotKey(hWndMain, HOTKEY_KILL);
    UnregisterHotKey(hWndMain, HOTKEY_XKILL);
    return 0;
}
