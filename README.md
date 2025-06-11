# 💥 Shatter

**Shatter** is a lightweight Windows utility that gives you fast, no-compromise control over unresponsive or stubborn applications. With a simple hotkey, you can instantly terminate any program — no task manager, no dialogs, no fuss.

---

## ⚙️ Features

- 🔥 **Instant-kill** hotkey: Terminate the currently focused window immediately
- 🎯 **Xkill mode**: Click any window on your screen to forcefully close it
- 🧼 Runs in the background without a visible window or tray icon
- 🪶 Minimal footprint — no dependencies, no installation needed

---

## 🖥️ Usage

| Hotkey            | Action                                 |
|------------------|----------------------------------------|
| `Ctrl + Alt + F4` | Kill the currently focused window      |
| `Win + F4`        | Enter xkill mode – click a window to kill it |

- In **xkill mode**:
  - **Left-click** to terminate a window
  - **Right-click** to cancel the action

---

## 🚀 Getting Started

### ✅ Requirements
- Windows 7 or later (x64)
- Administrator rights only if terminating protected processes

### 🛠️ Building from Source
1. Open the solution in **Visual Studio**
2. Set the subsystem to **Windows**:
   - `Project → Properties → Linker → System → Subsystem → Windows`
3. Build the project
4. Run `Shatter.exe`

---

## 🔧 Technical Notes

- Uses the Windows API (`RegisterHotKey`, `TerminateProcess`, etc.)
- Operates silently via hotkeys and system hooks
- Does **not** ask applications to close — it terminates their processes directly
- No background services, console windows, or GUI components required

---

## ⚠️ Caution

- **No warning or confirmation** — Shatter kills processes instantly
- May cause data loss in unsaved applications
- Avoid terminating critical system processes (e.g. `explorer.exe`, `winlogon.exe`)

Use responsibly.

---

## 📦 Roadmap Ideas

- [ ] Optional tray icon with exit control
- [ ] Config file for custom hotkeys
- [ ] Safety blacklist to prevent accidental system process termination
- [ ] Visual feedback in xkill mode
- [ ] Elevation handling for protected apps

---

## 📄 License

Shatter is released under a **custom Non-Commercial Share-Alike license**.

- ✅ Free to use, modify, and share
- ❌ Not allowed for commercial use or resale
- 🔁 Must share source and license if redistributed

See [`LICENSE.txt`](./LICENSE.txt) for full details.  
For commercial use, contact: daniel.coffey11@gmail.com

---
