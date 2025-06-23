# Shatter Installer

This folder contains the Inno Setup script and related resources used to build the Windows installer for **Shatter**.

---

## 📦 Contents

- `ShatterInstaller.iss` – The Inno Setup script used to compile the installer.
- `res/shatter.ico` – The icon used for the installer executable.

---

## 🛠 How to Build the Installer

1. Install [Inno Setup](https://jrsoftware.org/isinfo.php) if you haven’t already.
2. Open `ShatterInstaller.iss` in the **Inno Setup Compiler**.
3. Click **Build → Compile**.
4. The generated `ShatterInstaller.exe` will appear in this directory.

---

## 📁 Output

- `ShatterInstaller.exe` – The final signed or unsigned installer.
- This file is typically not committed to the repo, but is published in [GitHub Releases](https://github.com/dcoffey11/Shatter/releases).

---

## 💡 Notes

- **License Agreement**: Displays `LICENSE.txt` during install if present in the root.
- **Silent Install**: Supports silent install mode via `ShatterInstaller.exe /silent` or `/verysilent`.
- **Autostart Support**: Installer adds a registry entry to enable startup with Windows.
- **Uninstall Support**: Fully clean uninstall via Control Panel or Start Menu.

---

Created by [Daniel Coffey](mailto:daniel.coffey11@gmail.com)
