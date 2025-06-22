#include <windows.h>
#include <winuser.h>
#include <shellapi.h>
#include <psapi.h>
#include <string>
#include <map>
#include <commctrl.h>
#include "resource.h"

// Global map of key names to virtual key codes
static const std::map<std::wstring, UINT> keyMap = {
    // Function keys
    {L"F1", VK_F1}, {L"F2", VK_F2}, {L"F3", VK_F3}, {L"F4", VK_F4},
    {L"F5", VK_F5}, {L"F6", VK_F6}, {L"F7", VK_F7}, {L"F8", VK_F8},
    {L"F9", VK_F9}, {L"F10", VK_F10}, {L"F11", VK_F11}, {L"F12", VK_F12},
    // Numbers
    {L"0", '0'}, {L"1", '1'}, {L"2", '2'}, {L"3", '3'}, {L"4", '4'},
    {L"5", '5'}, {L"6", '6'}, {L"7", '7'}, {L"8", '8'}, {L"9", '9'},
    // Letters
    {L"A", 'A'}, {L"B", 'B'}, {L"C", 'C'}, {L"D", 'D'}, {L"E", 'E'},
    {L"F", 'F'}, {L"G", 'G'}, {L"H", 'H'}, {L"I", 'I'}, {L"J", 'J'},
    {L"K", 'K'}, {L"L", 'L'}, {L"M", 'M'}, {L"N", 'N'}, {L"O", 'O'},
    {L"P", 'P'}, {L"Q", 'Q'}, {L"R", 'R'}, {L"S", 'S'}, {L"T", 'T'},
    {L"U", 'U'}, {L"V", 'V'}, {L"W", 'W'}, {L"X", 'X'}, {L"Y", 'Y'},
    {L"Z", 'Z'},
    // Special keys
    {L"SPACE", VK_SPACE}, {L"ENTER", VK_RETURN}, {L"TAB", VK_TAB},
    {L"ESC", VK_ESCAPE}, {L"BACKSPACE", VK_BACK}, {L"DELETE", VK_DELETE},
    {L"INSERT", VK_INSERT}, {L"HOME", VK_HOME}, {L"END", VK_END},
    {L"PAGEUP", VK_PRIOR}, {L"PAGEDOWN", VK_NEXT},
    {L"UP", VK_UP}, {L"DOWN", VK_DOWN}, {L"LEFT", VK_LEFT}, {L"RIGHT", VK_RIGHT},
    // Numpad keys
    {L"NUMPAD0", VK_NUMPAD0}, {L"NUMPAD1", VK_NUMPAD1}, {L"NUMPAD2", VK_NUMPAD2},
    {L"NUMPAD3", VK_NUMPAD3}, {L"NUMPAD4", VK_NUMPAD4}, {L"NUMPAD5", VK_NUMPAD5},
    {L"NUMPAD6", VK_NUMPAD6}, {L"NUMPAD7", VK_NUMPAD7}, {L"NUMPAD8", VK_NUMPAD8},
    {L"NUMPAD9", VK_NUMPAD9}, {L"NUMPADMULTIPLY", VK_MULTIPLY}, {L"NUMPADADD", VK_ADD},
    {L"NUMPADSUBTRACT", VK_SUBTRACT}, {L"NUMPADDECIMAL", VK_DECIMAL},
    {L"NUMPADDIVIDE", VK_DIVIDE},
    // Other keys
    {L"SEMICOLON", VK_OEM_1}, {L"PLUS", VK_OEM_PLUS}, {L"COMMA", VK_OEM_COMMA},
    {L"MINUS", VK_OEM_MINUS}, {L"PERIOD", VK_OEM_PERIOD}, {L"SLASH", VK_OEM_2},
    {L"BACKTICK", VK_OEM_3}, {L"LBRACKET", VK_OEM_4}, {L"BACKSLASH", VK_OEM_5},
    {L"RBRACKET", VK_OEM_6}, {L"QUOTE", VK_OEM_7}
};

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_TOGGLE 1002
#define ID_TRAY_AUTOSTART 1003
#define ID_TRAY_CLICK_KILL 1004
#define ID_TRAY_CONFIGURE 1005

// Global Variables
HINSTANCE hInst;
HWND g_mainWindow = NULL;
NOTIFYICONDATA nid = {};
HMENU hTrayMenu = NULL;
bool enableXKill = true;
bool clickKillMode = false;
HHOOK mouseHook = NULL;
HCURSOR g_crosshairCursor = NULL;
HWND g_overlayWnd = NULL;

// Forward Declarations
void CreateOverlayWindow();
void DestroyOverlayWindow();
void ReRegisterHotkeys(HWND hwnd);
std::wstring ReadKeybindFromIni(const wchar_t* key, const wchar_t* def);
INT_PTR CALLBACK HotkeyDialogProc(HWND, UINT, WPARAM, LPARAM);

// Function Implementations

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

void ExitClickKillMode() {
    clickKillMode = false;
    CheckMenuItem(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_UNCHECKED);
    HCURSOR arrowCursor = LoadCursor(NULL, IDC_ARROW);
    SetCursor(arrowCursor);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    wcscpy_s(nid.szTip, L"Shatter");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    DestroyOverlayWindow();
    KillTimer(NULL, 1);
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = NULL;
    }
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && clickKillMode) {
        if (g_crosshairCursor) {
            SetCursor(g_crosshairCursor);
        }
        if (wParam == WM_LBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            HWND target = WindowFromPoint(pt);
            KillWindow(target);
            ExitClickKillMode();
        }
        else if (wParam == WM_RBUTTONDOWN) {
            ExitClickKillMode();
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void StartClickKill() {
    clickKillMode = true;
    CheckMenuItem(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_CHECKED);
    CreateOverlayWindow();
    g_crosshairCursor = LoadCursor(NULL, IDC_CROSS);
    SetCursor(g_crosshairCursor);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    wcscpy_s(nid.szInfo, L"Click-to-Kill mode activated! Click any window to close it.");
    wcscpy_s(nid.szInfoTitle, L"Shatter");
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInst, 0);
    SetTimer(NULL, 1, 50, NULL);
}

void ToggleEnableXKill() {
    enableXKill = !enableXKill;
    CheckMenuItem(hTrayMenu, ID_TRAY_TOGGLE, MF_BYCOMMAND | (enableXKill ? MF_CHECKED : MF_UNCHECKED));
}

void UpdateTrayMenu() {
    std::wstring clickKeybind = ReadKeybindFromIni(L"ClickKill", L"Ctrl+Alt+X");
    std::wstring menuItemText = L"Click-to-Kill (" + clickKeybind + L")";
    ModifyMenu(hTrayMenu, ID_TRAY_CLICK_KILL, MF_BYCOMMAND | MF_STRING, ID_TRAY_CLICK_KILL, menuItemText.c_str());
}

void InitTray(HWND hwnd) {
    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING | (enableXKill ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_TOGGLE, L"Enable XKill");
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_CLICK_KILL, L"Click-to-Kill (Ctrl+Alt+X)");
    AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_CONFIGURE, L"Configure Hotkeys...");
    AppendMenu(hTrayMenu, MF_STRING | (IsAutostartEnabled() ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_AUTOSTART, L"Start with Windows");
    AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    UpdateTrayMenu();
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
    const wchar_t OVERLAY_CLASS[] = L"ShatterOverlayClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (msg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rect;
                GetClientRect(hwnd, &rect);
                HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
                FrameRect(hdc, &rect, redBrush);
                DeleteObject(redBrush);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 0, 0));
                SetTextAlign(hdc, TA_CENTER | TA_TOP);
                const wchar_t* text = L"CLICK-TO-KILL MODE ACTIVE";
                TextOutW(hdc, rect.right / 2, 50, text, wcslen(text));
                EndPaint(hwnd, &ps);
                return 0;
            }
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    };
    wc.hInstance = hInst;
    wc.lpszClassName = OVERLAY_CLASS;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);
    g_overlayWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        OVERLAY_CLASS, L"Shatter Overlay", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInst, NULL
    );
    if (g_overlayWnd) {
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

void ParseKeybind(const std::wstring& keybind, UINT& modifiers, UINT& vk) {
    modifiers = 0;
    vk = 0;
    std::wstring s = keybind;
    for (auto& c : s) c = towupper(c);

    if (s.find(L"CTRL+") != std::wstring::npos) modifiers |= MOD_CONTROL;
    if (s.find(L"ALT+") != std::wstring::npos) modifiers |= MOD_ALT;
    if (s.find(L"SHIFT+") != std::wstring::npos) modifiers |= MOD_SHIFT;
    if (s.find(L"WIN+") != std::wstring::npos) modifiers |= MOD_WIN;

    size_t pos = s.rfind(L'+');
    std::wstring key = (pos != std::wstring::npos) ? s.substr(pos + 1) : s;

    auto it = keyMap.find(key);
    if (it != keyMap.end()) {
        vk = it->second;
    }
}

std::wstring ReadKeybindFromIni(const wchar_t* key, const wchar_t* def) {
    static const std::wstring iniPath = GetIniPath();
    wchar_t buf[64] = {0};
    GetPrivateProfileStringW(L"Hotkeys", key, def, buf, 64, iniPath.c_str());
    return buf;
}

std::wstring HotkeyToString(LPARAM hotkey) {
    UINT vk = LOWORD(hotkey);
    UINT modifiers = HIWORD(hotkey);
    std::wstring s = L"";
    if (modifiers & HOTKEYF_CONTROL) s += L"Ctrl+";
    if (modifiers & HOTKEYF_ALT) s += L"Alt+";
    if (modifiers & HOTKEYF_SHIFT) s += L"Shift+";
    
    for (const auto& pair : keyMap) {
        if (pair.second == vk) {
            s += pair.first;
            return s;
        }
    }
    if (iswalpha(vk) || iswdigit(vk)) {
        s += (wchar_t)vk;
    }
    return s;
}

UINT ConvertModToHotkeyf(UINT mod) {
    UINT newMod = 0;
    if (mod & MOD_CONTROL) newMod |= HOTKEYF_CONTROL;
    if (mod & MOD_ALT) newMod |= HOTKEYF_ALT;
    if (mod & MOD_SHIFT) newMod |= HOTKEYF_SHIFT;
    // Note: MOD_WIN is not supported by the hotkey control
    return newMod;
}

void ReRegisterHotkeys(HWND hwnd) {
    UnregisterHotKey(hwnd, 1);
    UnregisterHotKey(hwnd, 2);

    UINT mod1, vk1, mod2, vk2;
    ParseKeybind(ReadKeybindFromIni(L"KillForeground", L"Ctrl+Alt+F4"), mod1, vk1);
    ParseKeybind(ReadKeybindFromIni(L"ClickKill", L"Ctrl+Alt+X"), mod2, vk2);

    RegisterHotKey(hwnd, 1, mod1, vk1);
    RegisterHotKey(hwnd, 2, mod2, vk2);

    UpdateTrayMenu();
}

INT_PTR CALLBACK HotkeyDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        {
            UINT mod1, vk1, mod2, vk2;
            ParseKeybind(ReadKeybindFromIni(L"KillForeground", L"Ctrl+Alt+F4"), mod1, vk1);
            ParseKeybind(ReadKeybindFromIni(L"ClickKill", L"Ctrl+Alt+X"), mod2, vk2);

            // Convert the flags for display purposes
            UINT display_mod1 = ConvertModToHotkeyf(mod1);
            UINT display_mod2 = ConvertModToHotkeyf(mod2);

            // Manually construct the WPARAM according to the documentation
            // The key code is in the low byte of the low word.
            // The modifiers are in the high byte of the low word.
            WPARAM wparam1 = MAKEWORD(vk1, display_mod1);
            WPARAM wparam2 = MAKEWORD(vk2, display_mod2);

            SendDlgItemMessage(hwnd, IDC_HOTKEY_KILL, HKM_SETHOTKEY, wparam1, 0);
            SendDlgItemMessage(hwnd, IDC_HOTKEY_CLICK, HKM_SETHOTKEY, wparam2, 0);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            LRESULT killHotKey = SendDlgItemMessage(hwnd, IDC_HOTKEY_KILL, HKM_GETHOTKEY, 0, 0);
            LRESULT clickHotKey = SendDlgItemMessage(hwnd, IDC_HOTKEY_CLICK, HKM_GETHOTKEY, 0, 0);

            WritePrivateProfileStringW(L"Hotkeys", L"KillForeground", HotkeyToString(killHotKey).c_str(), GetIniPath().c_str());
            WritePrivateProfileStringW(L"Hotkeys", L"ClickKill", HotkeyToString(clickHotKey).c_str(), GetIniPath().c_str());
            
            ReRegisterHotkeys(g_mainWindow);

            EndDialog(hwnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            if (wParam == 1 && enableXKill) KillForegroundWindow();
            else if (wParam == 2) StartClickKill();
            break;
        case WM_TIMER:
            if (wParam == 1 && clickKillMode && g_crosshairCursor) {
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
                case ID_TRAY_CONFIGURE:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_HOTKEY_CONFIG), hwnd, HotkeyDialogProc);
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

    g_mainWindow = CreateWindowEx(0, CLASS_NAME, L"Shatter", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!g_mainWindow) return 1;

    ReRegisterHotkeys(g_mainWindow);
    InitTray(g_mainWindow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
