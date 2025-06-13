# Shatter

**Shatter** is a lightweight Windows tray application that adds "xkill"-style functionality to your PC.

## 🔥 Features

- 🖱️ Kill the foreground window with **Ctrl + Alt + F4**
- 🧠 Prevent killing protected processes like `explorer.exe`
- 🛠️ Enable/disable kill hotkey with a tray menu checkbox
- 🚀 Optional autostart with Windows via the registry
- 📁 INI-based configuration

## 📦 Installation

1. Download the latest release or build from source
2. Run `Shatter.exe`
3. Right-click the tray icon to configure options

## 🔧 Configuration

Located in `Shatter.ini`:

```ini
[Options]
EnableXKill=1
```

## 🗑️ Kill Hotkey

By default, `Ctrl + Alt + F4` will terminate the currently focused window — unless it's blacklisted (e.g. `explorer.exe`).

## 🪟 Autostart with Windows

Enable or disable from the tray menu:
✔️ Start with Windows

---

Made by Daniel Coffey  
📧 daniel.coffey11@gmail.com  
🔒 Custom license: personal, non-commercial use only
