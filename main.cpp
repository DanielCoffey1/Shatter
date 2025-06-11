#include <windows.h>
#include <tchar.h>

#define HOTKEY_KILL     1
#define HOTKEY_XKILL    2

HINSTANCE hInst;
HHOOK mouseHook;
bool inXKillMode = false;
HCURSOR killCursor;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (!inXKillMode || nCode < 0)
        return CallNextHookEx(NULL, nCode, wParam, lParam);

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
        // cancel xkill mode
        inXKillMode = false;
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        UnhookWindowsHookEx(mouseHook);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

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
    // Optional: Load a custom cursor like "shatter_kill.cur"
    killCursor = LoadCursorFromFile(_T("shatter_kill.cur"));
    if (!killCursor) killCursor = LoadCursor(NULL, IDC_CROSS);
    SetCursor(killCursor);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    hInst = hInstance;

    // Register hotkeys
    RegisterHotKey(NULL, HOTKEY_KILL, MOD_CONTROL | MOD_ALT, VK_F4);  // Ctrl+Alt+F4
    RegisterHotKey(NULL, HOTKEY_XKILL, MOD_WIN, VK_F4);               // Win+F4

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
            case HOTKEY_KILL:
                KillForegroundWindow();
                break;
            case HOTKEY_XKILL:
                StartXKillMode();
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    UnregisterHotKey(NULL, HOTKEY_KILL);
    UnregisterHotKey(NULL, HOTKEY_XKILL);
    return 0;
}
