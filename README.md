# 🎙️ CastIt

**Broadcast Streaming Control Panel - Streaming Server Statistics (C++ MFC, Win32)**  
_A fully-featured legacy Windows broadcasting tool originally built in Visual Studio 6. Being revived and modernized for Visual Studio 2022._

---

## 📂 Project Structure

- `legacy-source-vs6/` — Original C++ MFC source code from Visual Studio 6 (2005 era)
- Modern `.vcxproj` upgrade planned for Visual Studio 2022
- Installer support via Inno Setup script (`Castit.iss`)

---

## 🚀 Key Features

- 🎚️ Real-time broadcast UI for Shoutcast/Icecast/SteamCast
- 📁 Template-based HTML generation for web output
- 🗄️ MySQL backend support for logging and server data
- 🧰 SFTP integration, tray notifications, and modular UI design

---

## 🔧 Resurrection Plan

1. Convert `legacy-source-vs6/` from `.dsw/.dsp` → `.vcxproj` via VS2013 bridge
2. Rebuild in Visual Studio 2022 for x86/x64 targets
3. Update dependency paths for:
   - `libcurl`
   - `libxml2`
   - `mysqlclient`
4. Restore and modernize installer pipeline
5. Tag a 2025 re-release binary

---

## 🕰️ Legacy Notes

> Originally built and released in 2005 as part of the Mediacast1 / CasterClub streaming tool suite.  
> CastIt remains a testament to stable MFC engineering and real-time broadcast reliability.  
> Still functional on Windows 11 Pro today.

---

## 📜 License

Legacy proprietary code (2005). Re-release planned under MIT or GPLv3.  
Stay tuned for updated licensing and contribution guidelines.

---

## 🧠 Author

**David St. John** — Original developer and stream-hosting pioneer  
[LinkedIn](https://www.linkedin.com/in/davestj) • [Mediacast1.ai](https://mediacast1.ai) (coming soon)

