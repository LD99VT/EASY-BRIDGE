# Easy Bridge

**Easy Bridge** is a cross-platform desktop application for converting and routing timecode between different formats used in live production, broadcast, and post-production workflows.

Receive timecode in one format — send it out in another, simultaneously, with minimal latency.

---

## Supported formats

| Direction | LTC | MTC | ArtNet TC | OSC |
|-----------|:---:|:---:|:---------:|:---:|
| Input     | ✓   | ✓   | ✓         | ✓   |
| Output    | ✓   | ✓   | ✓         |     |

- **LTC** — Linear Timecode over audio (ASIO / WDM / CoreAudio)
- **MTC** — MIDI Timecode (any MIDI interface)
- **ArtNet TC** — Timecode over Ethernet via the ArtNet protocol
- **OSC** — Open Sound Control (receive timecode from OSC-capable software)

---

## Download

Pre-built installers are attached to each [GitHub Release](../../releases/latest):

| Platform | Asset |
|----------|-------|
| Windows  | `EasyBridge-win64.zip` — standalone `.exe` |
| macOS    | `EasyBridge-<version>.dmg` — universal binary (Intel + Apple Silicon) |

---

## Build from source

### Prerequisites

**Windows**

- Visual Studio 2022 (workload: *Desktop development with C++*)
- CMake 3.22+
- Git

```powershell
winget install Kitware.CMake Git.Git
```

- (Optional) [Steinberg ASIO SDK](https://www.steinberg.net/asiosdk/) for ASIO/ReaRoute support.
  Place it in `ASIOSDK/` inside the repo or set the `ASIO_SDK_DIR` environment variable.

**macOS**

- Xcode + Command Line Tools
- CMake 3.22+, Git

```bash
xcode-select --install
brew install cmake git
```

---

### Build (executable only)

**Windows:**
```powershell
./build_win.ps1
```

**macOS:**
```bash
chmod +x build_mac.sh
./build_mac.sh
```

Output:
- Windows: `build/windows-msvc/EasyBridge_artefacts/Release/Easy Bridge.exe`
- macOS: `build/macos-universal/EasyBridge_artefacts/Release/Easy Bridge.app`

---

### Build Windows installer

Requires [Inno Setup 6](https://jrsoftware.org/isinfo.php).

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer_bridge.iss
```

Output: `Installer/EasyBridge_Setup_<version>.exe`

---

## Versioning

Single source of truth: **`version.iss`** — field `AppVersion "X.Y.Z"`.

CMake reads it at configure time and generates `Source/core/BridgeVersion.h` from `Source/core/BridgeVersion.h.in`.
Inno Setup reads the same file for the installer filename and version metadata.

---

## Release (CI/CD)

Releases are built automatically by GitHub Actions (`.github/workflows/release.yml`).

**Trigger by tag:**
```bash
# Edit version.iss first, then:
git add version.iss
git commit -m "Bump version to X.Y.Z"
git tag vX.Y.Z
git push origin main --tags
```

**Trigger manually:** GitHub → Actions → *Build And Release* → *Run workflow*.

The workflow produces:
- `EasyBridge-win64.zip` — Windows exe (built on `windows-latest`)
- `EasyBridge-<version>.dmg` — macOS universal binary (built on `macos-latest`)

Both assets are uploaded as a GitHub Release when triggered by a tag.

---

## Project structure

```
EASYBRIDGE-JUSE/
├── Source/
│   ├── core/           # Timecode math, config, version header template
│   ├── engine/         # Input/output engines (LTC, MTC, ArtNet, OSC)
│   │   ├── inputs/
│   │   ├── outputs/
│   │   └── timecode/   # Low-level timecode processing
│   ├── ui/             # JUCE UI components and widgets
│   └── platform/       # OS-specific helpers (Windows dark title bar, icon)
├── Fonts/              # Bundled fonts (JetBrains Mono, Thunder)
├── Icons/              # App icons (icns, ico, png)
├── Help/               # Built-in HTML help
├── cmake/              # CMake helper scripts
├── CMakeLists.txt
├── CMakePresets.json
├── version.iss         # ← version lives here
├── installer_bridge.iss
├── build_win.ps1
└── build_mac.sh
```
