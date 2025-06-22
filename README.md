
# Shatter

**Shatter** is a lightweight Windows tray utility that brings powerful, Linux-style window-killing functionality to your PC. Inspired by `xkill`, it allows you to instantly terminate unresponsive applications with a simple hotkey.

## 🔥 Features

- **Kill Foreground Window**: Instantly close the active window.
  - Default Hotkey: `Ctrl+Alt+F4`
- **Click-to-Kill Mode**: Activate a special mode to select and terminate any window with a mouse click.
  - Default Hotkey: `Ctrl+Alt+X`
- **Customizable Hotkeys**: Change your hotkeys using a built-in configuration window or manually edit the INI file.
- **Process Protection**: Prevents you from accidentally killing critical system processes like `explorer.exe`.
- **Tray Integration**: Runs quietly in the system tray. Right-click the icon to enable/disable features or exit the application.
- **Autostart with Windows**: Conveniently set Shatter to start automatically when you log in.

## 📦 Installation

1. Download the latest **ShatterInstaller.exe** from the [Releases](https://github.com/dcoffey1/Shatter/releases) page.
2. Run the installer and follow the prompts.
3. Once installed, Shatter will start in your system tray.
4. (Optional) Right-click the tray icon and select "Start with Windows" to enable autostart.

> **Note**: If Windows SmartScreen blocks the installer, click **More Info → Run Anyway**. This is expected for unsigned apps.

## 🔧 Configuration & Hotkeys

Shatter provides two ways to configure your hotkeys:

### 1. Using the Configuration Dialog (Recommended)

1. Right-click the **Shatter** icon in your system tray.  
2. Select **"Configure Hotkeys..."**  
3. Click inside the input fields and press your desired key combinations  
4. Click **"OK"** to save. Your new hotkeys will be active immediately  

### 2. Editing the INI File

Shatter also uses a simple `.ini` file for manual configuration.

You can find it at:
```
C:\Users\<YourName>\AppData\Local\Shatter\Shatter.ini
```

Edit it like this:

```ini
[Hotkeys]
KillForeground=Ctrl+Alt+F4
ClickKill=Ctrl+Alt+X
```

Supported modifiers: `Ctrl`, `Alt`, `Shift`, `Win`.

After saving your changes, restart Shatter to apply them.

---

Made by Daniel Coffey  
📧 daniel.coffey11@gmail.com  
🔒 Licensed for personal, non-commercial use. See LICENSE.txt for details.
