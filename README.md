# Easy Bridge

**Easy Bridge** is a cross-platform desktop application for converting and routing timecode between different formats used in live production, broadcast, and post-production workflows.

Receive timecode in one format — send it out in another, simultaneously, with minimal latency.

## Screenshot

![Easy Bridge UI](ScreanShot/ScreanShot.png)

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
| Windows  | `EasyBridge_Setup_<version>.exe` — installer |
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

### Build

**Windows:**
```powershell
./build_win.ps1
```

**macOS:**
```bash
chmod +x build_mac.sh
./build_mac.sh
```
