#include <windows.h>
#include <winuser.h>
#include <shellapi.h>
#include <psapi.h>
#include <string>
#include "resource.h"

// Function to get the full path of the INI file located next to the executable
std::wstring GetIniPath() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    size_t pos = path.rfind(L'\\');
    if (pos != std::wstring::npos) {
        return path.substr(0, pos + 1) + L"Shatter.ini";
    }
    return L"Shatter.ini"; // Fallback
}

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_TOGGLE 1002
#define ID_TRAY_AUTOSTART 1003
#define ID_TRAY_CLICK_KILL 1004

HINSTANCE hInst;
NOTIFYICONDATA nid = {};
HMENU hTrayMenu = NULL;
bool enableXKill = true;
bool clickKillMode = false;
HHOOK mouseHook = NULL;
HHOOK cursorHook = NULL;
HCURSOR g_crosshairCursor = NULL;
HWND g_overlayWnd = NULL;

// Function declarations
void CreateOverlayWindow();
void DestroyOverlayWindow();

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
    } else {
        WCHAR exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);
        std::wstring quotedPath = L"\"" + std::wstring(exePath) + L"\"";
        RegSetValueExW(hKey, L"Shatter", 0, REG_SZ, (const BYTE*)quotedPath.c_str(),
                       static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);
}

bool IsBlacklisted(const std::wstring& exeName) {
    return exeName == L"explorer.exe";
}

void KillWindow(HWND hwnd) {
    if (!hwnd) return;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return;

    WCHAR path[MAX_PATH];
    GetModuleFileNameEx(hProcess, NULL, path, MAX_PATH);
    std::wstring exeName = std::wstring(path).substr(std::wstring(path).find_last_of(L"\\") + 1);

    if (!IsBlacklisted(exeName)) {
        TerminateProcess(hProcess, 0);
    }

    CloseHandle(hProcess);
}

void KillForegroundWindow() {
    HWND hwnd = GetForegroundWindow();
    KillWindow(hwnd);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && clickKillMode) {
        // Force the cursor to be our custom cursor on every mouse event
        if (g_crosshairCursor) {
            SetCursor(g_crosshairCursor);
        }
        
        if (wParam == WM_LBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            HWND target = WindowFromPoint(pt);
            KillWindow(target);
            clickKillMode = false;
            
            // Update tray menu to show click-kill mode is inactive
            CheckMenuItem(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_UNCHECKED);
            
            // Restore the normal arrow cursor
            HCURSOR arrowCursor = LoadCursor(NULL, IDC_ARROW);
            SetCursor(arrowCursor);
            
            // Restore normal notification
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            wcscpy_s(nid.szTip, L"Shatter");
            Shell_NotifyIcon(NIM_MODIFY, &nid);
            
            // Destroy overlay window
            DestroyOverlayWindow();
            
            // Kill the timer
            KillTimer(NULL, 1);
            
            UnhookWindowsHookEx(mouseHook);
            if (cursorHook) {
                UnhookWindowsHookEx(cursorHook);
                cursorHook = NULL;
            }
        }
        else if (wParam == WM_RBUTTONDOWN) {
            // Cancel click-kill mode with right-click
            clickKillMode = false;
            
            // Update tray menu to show click-kill mode is inactive
            CheckMenuItem(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_UNCHECKED);
            
            // Restore the normal arrow cursor
            HCURSOR arrowCursor = LoadCursor(NULL, IDC_ARROW);
            SetCursor(arrowCursor);
            
            // Restore normal notification
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            wcscpy_s(nid.szTip, L"Shatter");
            Shell_NotifyIcon(NIM_MODIFY, &nid);
            
            // Destroy overlay window
            DestroyOverlayWindow();
            
            // Kill the timer
            KillTimer(NULL, 1);
            
            UnhookWindowsHookEx(mouseHook);
            if (cursorHook) {
                UnhookWindowsHookEx(cursorHook);
                cursorHook = NULL;
            }
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK CursorHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && clickKillMode && g_crosshairCursor) {
        // Force crosshair cursor
        SetCursor(g_crosshairCursor);
    }
    return CallNextHookEx(cursorHook, nCode, wParam, lParam);
}

void StartClickKill() {
    clickKillMode = true;
    
    // Update tray menu to show click-kill mode is active
    CheckMenuItem(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_CHECKED);
    
    // Create overlay window to show visual indicator
    CreateOverlayWindow();
    
    // Try different cursor types - let's use a more aggressive approach
    // Try multiple cursor types to see which one is most visible
    g_crosshairCursor = LoadCursor(NULL, IDC_HAND); // Try hand cursor - very visible
    if (!g_crosshairCursor) {
        g_crosshairCursor = LoadCursor(NULL, IDC_SIZEALL); // Fallback to move cursor
    }
    if (!g_crosshairCursor) {
        g_crosshairCursor = LoadCursor(NULL, IDC_CROSS); // Fallback to crosshair
    }
    
    // Set cursor for our application window
    SetCursor(g_crosshairCursor);
    
    // Try to set cursor for the desktop window as well
    HWND desktopWnd = GetDesktopWindow();
    if (desktopWnd) {
        SetClassLongPtr(desktopWnd, GCLP_HCURSOR, (LONG_PTR)g_crosshairCursor);
    }
    
    // Force cursor update by moving it around more aggressively
    POINT pt;
    GetCursorPos(&pt);
    
    // Move cursor in a larger pattern to force redraw
    for (int i = 0; i < 10; i++) {
        SetCursorPos(pt.x + i, pt.y);
        SetCursor(g_crosshairCursor);
        Sleep(5);
    }
    for (int i = 0; i < 10; i++) {
        SetCursorPos(pt.x + 10 - i, pt.y);
        SetCursor(g_crosshairCursor);
        Sleep(5);
    }
    SetCursorPos(pt.x, pt.y);
    SetCursor(g_crosshairCursor);
    
    // Show a system notification to indicate click-kill mode is active
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    wcscpy_s(nid.szInfo, L"Click-to-Kill mode activated! Click any window to close it.");
    wcscpy_s(nid.szInfoTitle, L"Shatter");
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    
    // Set up the mouse hook to handle clicks
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInst, 0);
    
    // Set up a timer to continuously maintain the cursor
    SetTimer(NULL, 1, 50, NULL); // 50ms timer to maintain cursor more frequently
}

void ToggleEnableXKill() {
    enableXKill = !enableXKill;
    CheckMenuItem(hTrayMenu, ID_TRAY_TOGGLE, MF_BYCOMMAND | (enableXKill ? MF_CHECKED : MF_UNCHECKED));
}

void InitTray(HWND hwnd) {
    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING | (enableXKill ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_TOGGLE, L"Enable XKill");
    AppendMenu(hTrayMenu, MF_STRING | (clickKillMode ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_CLICK_KILL, L"Click-to-Kill (Ctrl+Alt+X)");
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

void CreateOverlayWindow() {
    if (g_overlayWnd) return;
    
    // Create a simple overlay window
    const wchar_t OVERLAY_CLASS[] = L"ShatterOverlayClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (msg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Draw a red border around the screen
                RECT rect;
                GetClientRect(hwnd, &rect);
                HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
                FrameRect(hdc, &rect, redBrush);
                DeleteObject(redBrush);
                
                // Draw text
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 0, 0));
                SetTextAlign(hdc, TA_CENTER | TA_TOP);
                
                const wchar_t* text = L"CLICK-TO-KILL MODE ACTIVE";
                TextOutW(hdc, rect.right / 2, 50, text, wcslen(text));
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                // Pass through clicks to underlying windows
                return DefWindowProc(hwnd, msg, wParam, lParam);
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    };
    wc.hInstance = hInst;
    wc.lpszClassName = OVERLAY_CLASS;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);
    
    // Create the overlay window
    g_overlayWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        OVERLAY_CLASS,
        L"Shatter Overlay",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInst, NULL
    );
    
    if (g_overlayWnd) {
        // Make it semi-transparent
        SetLayeredWindowAttributes(g_overlayWnd, 0, 128, LWA_ALPHA);
        ShowWindow(g_overlayWnd, SW_SHOW);
        UpdateWindow(g_overlayWnd);
    }
}

void DestroyOverlayWindow() {
    if (g_overlayWnd) {
        DestroyWindow(g_overlayWnd);
        g_overlayWnd = NULL;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            if (wParam == 1 && enableXKill) KillForegroundWindow();
            else if (wParam == 2) StartClickKill();
            break;
        case WM_TIMER:
            if (wParam == 1 && clickKillMode && g_crosshairCursor) {
                // Continuously maintain the cursor
                SetCursor(g_crosshairCursor);
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
                case ID_TRAY_CLICK_KILL:
                    StartClickKill();
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

// Helper to parse keybind string like "Ctrl+Alt+F4" into modifier and key
void ParseKeybind(const std::wstring& keybind, UINT& modifiers, UINT& vk) {
    modifiers = 0;
    vk = 0;
    std::wstring s = keybind;
    // Convert to uppercase for easier comparison
    for (auto& c : s) c = towupper(c);
    
    // Parse modifiers
    if (s.find(L"CTRL+") != std::wstring::npos) modifiers |= MOD_CONTROL;
    if (s.find(L"ALT+") != std::wstring::npos) modifiers |= MOD_ALT;
    if (s.find(L"SHIFT+") != std::wstring::npos) modifiers |= MOD_SHIFT;
    if (s.find(L"WIN+") != std::wstring::npos) modifiers |= MOD_WIN;
    
    // Find the key part (after last '+')
    size_t pos = s.rfind(L'+');
    std::wstring key = (pos != std::wstring::npos) ? s.substr(pos + 1) : s;
    
    // Function keys
    if (key == L"F1") vk = VK_F1;
    else if (key == L"F2") vk = VK_F2;
    else if (key == L"F3") vk = VK_F3;
    else if (key == L"F4") vk = VK_F4;
    else if (key == L"F5") vk = VK_F5;
    else if (key == L"F6") vk = VK_F6;
    else if (key == L"F7") vk = VK_F7;
    else if (key == L"F8") vk = VK_F8;
    else if (key == L"F9") vk = VK_F9;
    else if (key == L"F10") vk = VK_F10;
    else if (key == L"F11") vk = VK_F11;
    else if (key == L"F12") vk = VK_F12;
    
    // Numbers
    else if (key == L"0") vk = '0';
    else if (key == L"1") vk = '1';
    else if (key == L"2") vk = '2';
    else if (key == L"3") vk = '3';
    else if (key == L"4") vk = '4';
    else if (key == L"5") vk = '5';
    else if (key == L"6") vk = '6';
    else if (key == L"7") vk = '7';
    else if (key == L"8") vk = '8';
    else if (key == L"9") vk = '9';
    
    // Letters
    else if (key == L"A") vk = 'A';
    else if (key == L"B") vk = 'B';
    else if (key == L"C") vk = 'C';
    else if (key == L"D") vk = 'D';
    else if (key == L"E") vk = 'E';
    else if (key == L"F") vk = 'F';
    else if (key == L"G") vk = 'G';
    else if (key == L"H") vk = 'H';
    else if (key == L"I") vk = 'I';
    else if (key == L"J") vk = 'J';
    else if (key == L"K") vk = 'K';
    else if (key == L"L") vk = 'L';
    else if (key == L"M") vk = 'M';
    else if (key == L"N") vk = 'N';
    else if (key == L"O") vk = 'O';
    else if (key == L"P") vk = 'P';
    else if (key == L"Q") vk = 'Q';
    else if (key == L"R") vk = 'R';
    else if (key == L"S") vk = 'S';
    else if (key == L"T") vk = 'T';
    else if (key == L"U") vk = 'U';
    else if (key == L"V") vk = 'V';
    else if (key == L"W") vk = 'W';
    else if (key == L"X") vk = 'X';
    else if (key == L"Y") vk = 'Y';
    else if (key == L"Z") vk = 'Z';
    
    // Special keys
    else if (key == L"SPACE") vk = VK_SPACE;
    else if (key == L"ENTER") vk = VK_RETURN;
    else if (key == L"TAB") vk = VK_TAB;
    else if (key == L"ESC") vk = VK_ESCAPE;
    else if (key == L"BACKSPACE") vk = VK_BACK;
    else if (key == L"DELETE") vk = VK_DELETE;
    else if (key == L"INSERT") vk = VK_INSERT;
    else if (key == L"HOME") vk = VK_HOME;
    else if (key == L"END") vk = VK_END;
    else if (key == L"PAGEUP") vk = VK_PRIOR;
    else if (key == L"PAGEDOWN") vk = VK_NEXT;
    else if (key == L"UP") vk = VK_UP;
    else if (key == L"DOWN") vk = VK_DOWN;
    else if (key == L"LEFT") vk = VK_LEFT;
    else if (key == L"RIGHT") vk = VK_RIGHT;
    
    // Numpad keys
    else if (key == L"NUMPAD0") vk = VK_NUMPAD0;
    else if (key == L"NUMPAD1") vk = VK_NUMPAD1;
    else if (key == L"NUMPAD2") vk = VK_NUMPAD2;
    else if (key == L"NUMPAD3") vk = VK_NUMPAD3;
    else if (key == L"NUMPAD4") vk = VK_NUMPAD4;
    else if (key == L"NUMPAD5") vk = VK_NUMPAD5;
    else if (key == L"NUMPAD6") vk = VK_NUMPAD6;
    else if (key == L"NUMPAD7") vk = VK_NUMPAD7;
    else if (key == L"NUMPAD8") vk = VK_NUMPAD8;
    else if (key == L"NUMPAD9") vk = VK_NUMPAD9;
    else if (key == L"NUMPADMULTIPLY") vk = VK_MULTIPLY;
    else if (key == L"NUMPADADD") vk = VK_ADD;
    else if (key == L"NUMPADSUBTRACT") vk = VK_SUBTRACT;
    else if (key == L"NUMPADDECIMAL") vk = VK_DECIMAL;
    else if (key == L"NUMPADDIVIDE") vk = VK_DIVIDE;
    
    // Other keys
    else if (key == L"SEMICOLON") vk = VK_OEM_1;
    else if (key == L"PLUS") vk = VK_OEM_PLUS;
    else if (key == L"COMMA") vk = VK_OEM_COMMA;
    else if (key == L"MINUS") vk = VK_OEM_MINUS;
    else if (key == L"PERIOD") vk = VK_OEM_PERIOD;
    else if (key == L"SLASH") vk = VK_OEM_2;
    else if (key == L"BACKTICK") vk = VK_OEM_3;
    else if (key == L"LBRACKET") vk = VK_OEM_4;
    else if (key == L"BACKSLASH") vk = VK_OEM_5;
    else if (key == L"RBRACKET") vk = VK_OEM_6;
    else if (key == L"QUOTE") vk = VK_OEM_7;
}

// Helper to read a keybind from INI
std::wstring ReadKeybindFromIni(const wchar_t* key, const wchar_t* def) {
    static const std::wstring iniPath = GetIniPath();
    wchar_t buf[64] = {0};
    GetPrivateProfileStringW(L"Hotkeys", key, def, buf, 64, iniPath.c_str());
    return buf;
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

    // Read and parse keybinds from INI
    UINT mod1, vk1, mod2, vk2;
    std::wstring killKeybind = ReadKeybindFromIni(L"KillForeground", L"Ctrl+Alt+F4");
    std::wstring clickKeybind = ReadKeybindFromIni(L"ClickKill", L"Ctrl+Alt+X");
    
    ParseKeybind(killKeybind, mod1, vk1);
    ParseKeybind(clickKeybind, mod2, vk2);

    RegisterHotKey(hwnd, 1, mod1, vk1); // KillForeground
    RegisterHotKey(hwnd, 2, mod2, vk2); // ClickKill

    InitTray(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
