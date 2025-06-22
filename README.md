# Shatter

**Shatter** is a lightweight Windows tray utility that brings powerful, Linux-style window-killing functionality to your PC. Inspired by `xkill`, it allows you to instantly terminate unresponsive applications with a simple hotkey.

## 🔥 Features

- **Kill Foreground Window**: Instantly close the active window.
  - Default Hotkey: `Ctrl+Alt+F4`
- **Click-to-Kill Mode**: Activate a special mode to select and terminate any window with a mouse click.
  - Default Hotkey: `Ctrl+Alt+X`
  - A red overlay appears to clearly indicate that the mode is active.
- **Customizable Hotkeys**: Easily change the default keybinds by editing the `Shatter.ini` file. Supports a wide range of keys and modifiers.
- **Process Protection**: Prevents you from accidentally killing critical system processes like `explorer.exe`.
- **Tray Integration**: Runs quietly in the system tray. Right-click the icon to enable/disable features or exit the application.
- **Autostart with Windows**: Conveniently set Shatter to start automatically when you log in.

## 📦 Installation

1.  Download the latest `Shatter.exe` from the [Releases](https://github.com/dcoffey1/Shatter/releases) page.
2.  Place `Shatter.exe` and `Shatter.ini` in the same folder.
3.  Run `Shatter.exe`.
4.  (Optional) Right-click the tray icon and select "Start with Windows" to enable autostart.

## 🔧 Configuration & Hotkeys

You can customize Shatter's hotkeys by editing the `Shatter.ini` file located in the same directory as the executable.

```ini
[Hotkeys]
; Format: Modifier1+Modifier2+Key (e.g., Ctrl+Alt+F4)
; Supported modifiers: Ctrl, Alt, Shift, Win
; See the INI file for a full list of supported keys.
KillForeground=Ctrl+Alt+F4
ClickKill=Ctrl+Alt+X
```

Simply change the key strings, save the file, and restart the application for your new hotkeys to take effect.

---

Made by Daniel Coffey  
📧 daniel.coffey11@gmail.com  
🔒 Licensed for personal, non-commercial use. See LICENSE.txt for details.
