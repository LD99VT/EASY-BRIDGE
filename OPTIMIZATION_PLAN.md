# EasyBridge – Full Code Review & Optimization Plan

> **Date:** 2026-02-28  
> **Reviewed files:** all files under `Source/` and `CMakeLists.txt`

---

## Table of Contents

1. [Critical Bugs & Security](#1-critical-bugs--security)
2. [Correctness Issues](#2-correctness-issues)
3. [Performance Optimizations](#3-performance-optimizations)
4. [Architecture & Maintainability](#4-architecture--maintainability)
5. [Build & Project Hygiene](#5-build--project-hygiene)
6. [Style & Minor Improvements](#6-style--minor-improvements)
7. [Summary Checklist](#7-summary-checklist)

---

## 1. Critical Bugs & Security

### 1.1 OSC debug logging was always enabled ✅ FIXED

**File:** `Source/engine/OscInput.cpp`

**Problem:**  
`OscInput::start()` unconditionally set `debugEnabled_ = true` and appended timecode
data and raw UDP statistics to `/tmp/easybridge_osc_debug.log` on every session.
This meant:
- All received timecode values were persisted to disk continuously.
- The log file grew without bound (no rotation, no size cap).
- The file persisted across reboots, exposing production TC data unintentionally.

**Fix applied:**  
Removed the three debug fields (`debugLog_`, `debugEnabled_`, `lastDebugPacketTsMs_`)
from `OscInput.h` entirely, and removed all associated file-write calls from
`OscInput.cpp`. Debug logging no longer exists in the OSC input path.

**Instructions if debug logging is needed in future:**  
Add a compile-time or runtime opt-in (`#ifdef EASYBRIDGE_OSC_DEBUG` or a settings
checkbox) and limit the file to a fixed maximum size (e.g. 1 MB with rotation).

---

## 2. Correctness Issues

### 2.1 `App::moreThanOneInstanceAllowed()` returns `true`

**File:** `Source/App.h`

**Problem:**  
Returning `true` allows multiple application instances to run simultaneously. Because
each instance opens audio devices (LTC in/out) and binds UDP ports (ArtNet, OSC), a
second instance will silently compete for the same hardware, causing:
- Audio device access failures (device already in use).
- UDP port bind failures on the same port.
- Two conflicting timecode signals on the same network.

**Instructions:**
```cpp
// Source/App.h  line 14
bool moreThanOneInstanceAllowed() override { return false; }
```

---

### 2.2 `BridgeEngine::startMtcInput` skips `sameConfig` guard inconsistently

**File:** `Source/engine/BridgeEngine.cpp`, lines 121–134

**Problem:**  
The `sameConfig` guard for MTC input does not include an `errorOut.clear()` when
`sameConfig == true` and the device is already running. If the caller checks `errorOut`
after a no-op re-start it may see a stale error string from a previous failure.

**Instructions:**
```cpp
bool BridgeEngine::startMtcInput(int deviceIndex, juce::String& errorOut)
{
    const bool sameConfig = mtcInput_.getIsRunning() && deviceIndex == mtcInDeviceIndex_;
    if (sameConfig)
    {
        errorOut.clear();   // ← add this
        return true;
    }
    mtcInput_.refreshDeviceList();
    if (!mtcInput_.start(deviceIndex))
    {
        errorOut = "Failed to start MTC input";
        return false;
    }
    mtcInDeviceIndex_ = deviceIndex;
    errorOut.clear();
    return true;
}
```

---

### 2.3 `ArtnetInput` buffer size is too small for jumbo frames

**File:** `Source/engine/vendor/timecode/EngineArtnetInput.h`, line 133

**Problem:**  
`uint8_t buffer[1024]` is sufficient for the 19-byte ArtTimeCode packet but may truncate
other Art-Net packet types if the port receives them first (e.g. ArtPoll at ~14 bytes is
fine, but ArtDMX can be up to 530 bytes). A malformed oversized packet can cause
`socket->read()` to silently truncate and `parseArtNetPacket` will still process the
garbled prefix if the header bytes happen to be valid.

**Instructions:**  
Increase the receive buffer to 600 bytes (the maximum valid Art-Net v4 packet size) and
add an explicit length guard before processing:

```cpp
// EngineArtnetInput.h  run()
uint8_t buffer[600];
...
int bytesRead = socket->read(buffer, sizeof(buffer), false);
if (bytesRead < 19 || bytesRead > (int)sizeof(buffer))
    continue;   // too short or truncated
```

---

### 2.4 `ResolumeClipCollector` uses wrong namespace and is not compiled

**File:** `Source/engine/ResolumeClipCollector.h` / `.cpp`

**Problem:**  
The class is declared inside `namespace trigger::engine` but the rest of the project
uses `namespace bridge::engine`. Additionally, neither file is listed in
`CMakeLists.txt`, so the class is dead code that is never compiled and never tested.

**Instructions (choose one):**

*Option A – Remove (if the feature is not planned):*
```bash
git rm Source/engine/ResolumeClipCollector.h Source/engine/ResolumeClipCollector.cpp
```

*Option B – Fix and integrate:*
1. Rename namespace from `trigger::engine` to `bridge::engine` in both files.
2. Add to `CMakeLists.txt` `target_sources`:
   ```cmake
   Source/engine/ResolumeClipCollector.cpp
   Source/engine/ResolumeClipCollector.h
   ```

---

### 2.5 Dual `Timecode` / `FrameRate` type definitions

**Files:** `Source/core/Timecode.h` and `Source/engine/vendor/timecode/EngineTimecodeCore.h`

**Problem:**  
Two completely separate `Timecode` structs and `FrameRate` enumerations exist:
- `bridge::core::Timecode` / `bridge::core::FrameRateType` (integer enum)
- `::Timecode` / `::FrameRate` (scoped enum class, in global namespace)

The engine layer uses the global types. The core layer uses its own types. Converting
between them is implicit (via `BridgeEngine.h`'s using-declarations and type aliases in
the inputs/outputs headers), making the boundary fragile and confusing for new
contributors.

**Instructions (longer-term refactor):**
1. Decide on one canonical definition — the scoped `FrameRate` enum class in
   `EngineTimecodeCore.h` is the more modern, type-safe choice.
2. Replace `bridge::core::FrameRateType` integer comparisons with the enum class.
3. Keep `bridge::core::Timecode` as a thin wrapper or alias:
   ```cpp
   // core/Timecode.h
   using Timecode = ::Timecode;
   using FrameRateType = ::FrameRate;
   ```
4. Delete the now-redundant integer conversion helpers
   (`normalizeFpsTypeLegacy4`, `fpsNominalFrames`, `fpsTypeToMtcArtnetCode`, etc.) from
   `core/Timecode.cpp` once no callers remain.

---

## 3. Performance Optimizations

### 3.1 Duplicate atomic load in `OscInput::parseFloatTime` ✅ FIXED

**File:** `Source/engine/OscInput.cpp`

**Problem:**  
```cpp
const auto fps    = frameRateToDouble(fps_.load(std::memory_order_relaxed));
const int  fpsInt = juce::jmax(1, frameRateToInt(fps_.load(std::memory_order_relaxed)));
```
The atomic `fps_` is loaded twice from the same relaxed store. Although inexpensive,
this is unnecessary and risks reading two different values if a writer races between
the two loads (extremely unlikely but logically incorrect).

**Fix applied:**
```cpp
const auto fpsEnum = fps_.load(std::memory_order_relaxed);
const auto fps     = frameRateToDouble(fpsEnum);
const int  fpsInt  = juce::jmax(1, frameRateToInt(fpsEnum));
```

---

### 3.2 `BridgeEngine` same-config check uses string concatenation

**File:** `Source/engine/BridgeEngine.cpp`, line 301

**Problem:**  
```cpp
normalizedTargets.joinIntoString("|") == artnetOutTargets_.joinIntoString("|")
```
This allocates two heap strings on every config-check call just to compare lists. The
lists can be compared element-by-element at zero allocation cost.

**Instructions:**
```cpp
// Helper lambda or free function:
auto arraysEqual = [](const juce::StringArray& a, const juce::StringArray& b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) return false;
    return true;
};

const bool sameConfig =
    artnetOutput_.getIsRunning()
    && interfaceIndex == artnetOutInterfaceIndex_
    && arraysEqual(normalizedTargets, artnetOutTargets_);
```

---

### 3.3 `LtcInput` audio callback loads channel atomics inside hot loop

**File:** `Source/engine/vendor/timecode/EngineLtcInput.h`, `audioDeviceIOCallbackWithContext`

**Problem:**  
`selectedChannel.load()` and `passthruChannel.load()` are called inside (or just before)
the per-buffer audio callback. While one load is acceptable, the passthrough path also
loads `passthruGain` inside the inner sample loop implicitly via repeated reads. More
importantly, `samplesSinceLastSync += 1.0` accumulates as a `double` per sample, which
is three floating-point operations instead of one integer increment.

**Instructions:**
1. Snapshot all atomics once at the top of `audioDeviceIOCallbackWithContext`:
   ```cpp
   const int sCh     = selectedChannel.load(std::memory_order_relaxed);
   const int pCh     = passthruChannel.load(std::memory_order_relaxed);
   const float gain  = inputGain.load(std::memory_order_relaxed);
   const float pGain = passthruGain.load(std::memory_order_relaxed);
   ```
2. Change `samplesSinceLastSync` from `double` to `int64_t` and only convert to
   `double` for the comparison against `currentSampleRate * 2.0`:
   ```cpp
   int64_t samplesSinceLastSync = 0;
   ...
   samplesSinceLastSync++;
   ...
   if (samplesSinceLastSync > 0
       && samplesSinceLastSync < (int64_t)(currentSampleRate * 2.0))
   ```

---

### 3.4 `scanAudioDevices` creates a temporary `AudioDeviceManager` off the message thread

**File:** `Source/engine/BridgeEngine.cpp`, lines 10–33

**Problem:**  
`BridgeEngine::scanAudioInputs()` and `scanAudioOutputs()` each create a temporary
`juce::AudioDeviceManager`, call `initialise()`, and scan for devices. This can trigger
CoreAudio / WASAPI device enumeration from a background thread, which is not always
safe. Additionally, each call takes 100–500 ms, blocking the calling thread.

The UI currently avoids this with `BridgeAudioScanThread` (a dedicated thread), but
the scan itself still happens inside `scanAudioDevices()` without any thread affinity
guarantee from JUCE.

**Instructions:**
1. Ensure `scanAudioDevices` is only ever called from a background thread (already done
   via `BridgeAudioScanThread`).
2. Guard the scan with a flag so the main thread never calls it directly:
   ```cpp
   jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
   ```
3. For macOS: use `dispatch_async(dispatch_get_main_queue(), ...)` wrapping if
   CoreAudio APIs must run on the main thread (check JUCE docs for your version).

---

### 3.5 `ArtnetOutput::hiResTimerCallback` runs at 1 ms regardless of frame rate

**File:** `Source/engine/vendor/timecode/EngineArtnetOutput.h`, `updateTimerRate()`

**Problem:**  
The timer always fires at 1 ms intervals (1000 Hz) and uses a fractional accumulator to
determine when to actually send a packet. At 25 fps the true send interval is 40 ms, so
the timer fires ~40 times per packet—39 of those callbacks do nothing useful but still
wake the CPU.

**Instructions:**  
Schedule the timer at roughly half the expected frame interval so it fires ~2 × per
frame rather than 40 ×:
```cpp
void updateTimerRate()
{
    FrameRate fps = currentFps.load(std::memory_order_relaxed);
    double intervalMs = 1000.0 / frameRateToDouble(fps);
    // Fire at half the frame interval for 2× oversampling (catches jitter)
    int timerMs = juce::jmax(1, (int)(intervalMs * 0.5));
    lastFrameSendTime.store(juce::Time::getMillisecondCounterHiRes(),
                            std::memory_order_relaxed);
    startTimer(timerMs);
}
```
Apply the same change to `EngineMtcOutput.h::updateTimerRate()` (QF interval is
`1000 / (fps * 4)` ms, so fire at ~half that).

---

## 4. Architecture & Maintainability

### 4.1 `MainContentComponent` has 75+ member variables

**File:** `Source/MainWindow.h` / `Source/MainWindow.cpp`

**Problem:**  
`MainContentComponent` declares over 75 UI component members alongside UI state flags,
threading primitives, and business logic helpers—all in a single class. This makes the
file ~1700 lines long and very difficult to navigate, test, or extend.

**Instructions (incremental refactor, one step at a time):**

**Step 1 – Extract `LtcOutputApplyThread`**  
Move the `ltcOutputApplyThread_`, `ltcOutputApplyMutex_`, `ltcOutputApplyCv_`, and
related pending-state fields into a small `LtcOutputApplyThread` helper class.

**Step 2 – Extract `SourceSection`**  
Group the LTC/MTC/ArtNet/OSC input controls (labels + combos + editors) into a
`SourceSection : public juce::Component` with its own `resized()` and a callback
delegate:
```cpp
class SourceSection final : public juce::Component {
public:
    std::function<void()> onSettingsChanged;
    // comboboxes, labels, etc.
};
```

**Step 3 – Extract `OutputSection`**  
Same pattern for LTC-out, MTC-out, ArtNet-out.

**Step 4 – Move `BridgeLookAndFeel` to its own header**  
`BridgeLookAndFeel` is 80 lines and completely independent of `MainContentComponent`.
Move it to `Source/ui/BridgeLookAndFeel.h`.

---

### 4.2 All vendor engine classes are header-only

**Files:** `Source/engine/vendor/timecode/Engine*.h`

**Problem:**  
`EngineLtcInput.h` is ~486 lines, `EngineLtcOutput.h` ~370 lines, etc. Because these
are `#pragma once` headers included by `BridgeEngine.h` (which is included by
`MainWindow.h`), the full implementation is compiled into every translation unit that
transitively includes `MainWindow.h`. This significantly increases incremental build
times.

**Instructions:**
1. For each vendor class, create a corresponding `.cpp` file under
   `Source/engine/vendor/timecode/`.
2. Move the private method bodies into the `.cpp`, keeping only declarations and
   `inline` helper functions in the header.
3. Add the new `.cpp` files to `target_sources` in `CMakeLists.txt`.

---

### 4.3 `EngineTimecodeCore.h` defines types in global namespace

**File:** `Source/engine/vendor/timecode/EngineTimecodeCore.h`

**Problem:**  
`struct Timecode`, `enum class FrameRate`, and free functions (`packTimecode`,
`incrementFrame`, `frameRateToDouble`, etc.) are all defined at global scope. This
risks name collision with other libraries or system headers that define similar names.

**Instructions:**
1. Wrap everything in `namespace bridge::engine`:
   ```cpp
   namespace bridge::engine {
   enum class FrameRate { ... };
   struct Timecode { ... };
   // ... all helper functions ...
   } // namespace bridge::engine
   ```
2. Update all callers to use the qualified name or add `using namespace bridge::engine`
   in `.cpp` files only (never in headers).

---

### 4.4 `ClockState` in `Source/core/` is unused by the engine layer

**Files:** `Source/core/ClockState.h` / `ClockState.cpp`

**Problem:**  
`ClockState` provides a monotonic interpolation model (`nowTc()`) that extrapolates
timecode forward from a sync point using wall-clock elapsed time. However, the engine
inputs (`LtcInput`, `MtcInput`, etc.) maintain their own interpolation logic and
`BridgeEngine::tick()` reads raw timecodes without calling `ClockState`. The class is
only included in the build (via `CMakeLists.txt`) but never instantiated.

**Instructions:**
- Either wire `ClockState` into `BridgeEngine::tick()` as the interpolation layer
  (removing the duplicated logic in `MtcInput::getCurrentTimecode()`), or remove it
  from the build if it is genuinely not needed.

---

## 5. Build & Project Hygiene

### 5.1 `ResolumeClipCollector` is not listed in `CMakeLists.txt`

**File:** `CMakeLists.txt`

**Problem:**  
`Source/engine/ResolumeClipCollector.cpp` and `.h` exist on disk but are absent from
`target_sources`. They are never compiled and are silently ignored by CMake.

**Instructions:** See §2.4 above.

---

### 5.2 `CMakeLists.txt` does not list all engine/inputs headers

**File:** `CMakeLists.txt`

**Problem:**  
Headers under `Source/engine/inputs/`, `Source/engine/outputs/`, and
`Source/engine/vendor/timecode/` are not listed in `target_sources`. CMake doesn't
require listing headers, but IDEs (VS, CLion, Xcode) rely on this list to show them in
the project tree. Missing headers are invisible in the IDE file navigator.

**Instructions:**  
Add all headers to `target_sources` as non-compiled entries (CMake accepts headers
in the list, it just won't compile them):
```cmake
target_sources(EasyBridge PRIVATE
  # ... existing entries ...
  Source/engine/inputs/ArtnetInput.h
  Source/engine/inputs/LtcInput.h
  Source/engine/inputs/MtcInput.h
  Source/engine/inputs/OscInput.h
  Source/engine/outputs/ArtnetOutput.h
  Source/engine/outputs/LtcOutput.h
  Source/engine/outputs/MtcOutput.h
  Source/engine/vendor/timecode/EngineArtnetInput.h
  Source/engine/vendor/timecode/EngineArtnetOutput.h
  Source/engine/vendor/timecode/EngineLtcInput.h
  Source/engine/vendor/timecode/EngineLtcOutput.h
  Source/engine/vendor/timecode/EngineMtcInput.h
  Source/engine/vendor/timecode/EngineMtcOutput.h
  Source/engine/vendor/timecode/EngineNetworkUtils.h
  Source/engine/vendor/timecode/EngineTimecodeCore.h
)
```

---

### 5.3 `#pragma comment(lib, ...)` in headers

**File:** `Source/engine/vendor/timecode/EngineNetworkUtils.h`, lines 7–9  
**File:** `Source/MainWindow.cpp`, line 8

**Problem:**  
`#pragma comment(lib, "iphlpapi.lib")`, `#pragma comment(lib, "ws2_32.lib")`, and
`#pragma comment(lib, "dwmapi.lib")` are MSVC-specific pragmas placed inside headers /
source files rather than in the CMake build description. This couples the build to a
specific compiler toolchain.

**Instructions:**  
Remove the `#pragma comment(lib, ...)` directives and add the corresponding libraries
to `CMakeLists.txt`:
```cmake
if(WIN32)
  target_link_libraries(EasyBridge PRIVATE ws2_32 iphlpapi dwmapi)
endif()
```
`ws2_32` is already linked; add `iphlpapi` and `dwmapi`.

---

### 5.4 Hard-coded ASIO header check in CMakeLists

**File:** `CMakeLists.txt`, lines 71–76

**Problem:**  
The ASIO SDK detection loop checks for `common/asio.h` AND `host/asiodrivers.h` but
only `common/asio.h` is strictly required for JUCE ASIO support. The check for
`asiodrivers.h` causes false negatives when using certain SDK redistributions.

**Instructions:**  
Relax the check to require only `common/asio.h`:
```cmake
if(EXISTS "${_cand}/common/asio.h")
  set(ASIO_SDK_DIR "${_cand}" CACHE PATH "..." FORCE)
  break()
endif()
```

---

## 6. Style & Minor Improvements

### 6.1 `OscInput`: `addrLower` computed unconditionally

**File:** `Source/engine/OscInput.cpp`, `processDecodedMessage()`

**Problem:**  
```cpp
const auto addrLower = address.toLowerCase();
```
This is computed at the top of the function, but the two exact-match comparisons
(`address == addrStr_` and `address == addrFloat_`) will return early in the vast
majority of packets, meaning `addrLower` is only needed in the rare fallback path.

**Instructions:**  
Move the `toLowerCase()` call to just before the fallback pattern matching:
```cpp
bool OscInput::processDecodedMessage(...)
{
    if (address == addrStr_)    { ... return true/false; }
    if (address == addrFloat_)  { ... return true/false; }

    // Only compute lowercase for the less common fallback patterns
    const auto addrLower = address.toLowerCase();
    if (addrLower.contains("time") || ...)  { ... }
    if (addrLower.contains("frame") || ...) { ... }
    return false;
}
```

---

### 6.2 `BridgeLookAndFeel::drawPopupMenuBackground` ignores `width`/`height` parameters

**File:** `Source/MainWindow.h`, lines 231–234

**Problem:**  
The `width` and `height` parameters are unused. JUCE provides `g.fillAll()` which is
already correct for filling the full paint area, so this function is equivalent to the
default implementation. Remove it or add `juce::ignoreUnused(width, height)` for
documentation.

**Instructions:**  
```cpp
void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
{
    juce::ignoreUnused(width, height);
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
}
```

---

### 6.3 BOM characters in vendor headers

**Files:** `EngineLtcInput.h`, `EngineLtcOutput.h`, `EngineMtcInput.h`,
`EngineMtcOutput.h`, `EngineArtnetInput.h`, `EngineArtnetOutput.h`,
`EngineNetworkUtils.h`, `EngineTimecodeCore.h`

**Problem:**  
Each file starts with a UTF-8 BOM (`﻿`). While most compilers silently accept this,
BOM-prefixed files can cause issues with:
- `cat`/`diff`/`grep` producing unexpected output.
- Some CI tools that fail on BOM.
- Static analysis tools that misidentify the encoding.

**Instructions:**  
Remove the BOM from each file. In VS Code: open the file → click the encoding label in
the status bar → select "Save with Encoding" → choose "UTF-8" (without BOM).  
Or with `sed`:
```bash
for f in Source/engine/vendor/timecode/Engine*.h; do
  sed -i '1s/^\xEF\xBB\xBF//' "$f"
done
```

---

### 6.4 `README.md` describes the wrong product

**File:** `README.md`

**Problem:**  
The README header says "Easy Trigger JUCE" and the produced assets section refers to
`EasyTrigger-win64.zip` — but this repository is **EasyBridge**. The README was
apparently copied from a sibling project and not updated.

**Instructions:**  
1. Replace "Easy Trigger JUCE" with "Easy Bridge JUCE".
2. Replace all `EasyTrigger-*` asset names with `EasyBridge-*`.
3. Update the project description to mention LTC/MTC/ArtNet/OSC timecode bridging.

---

## 7. Summary Checklist

| # | Severity | Status | Item |
|---|----------|--------|------|
| 1.1 | 🔴 Critical | ✅ Fixed | OSC debug logging always enabled |
| 2.1 | 🔴 Critical | ⬜ Open | `moreThanOneInstanceAllowed()` returns `true` |
| 2.4 | 🟠 High | ⬜ Open | `ResolumeClipCollector` dead code / wrong namespace |
| 2.5 | 🟠 High | ⬜ Open | Dual `Timecode`/`FrameRate` type system |
| 3.1 | 🟡 Medium | ✅ Fixed | Duplicate atomic load in `parseFloatTime` |
| 2.2 | 🟡 Medium | ⬜ Open | `startMtcInput` sameConfig early-return missing `errorOut.clear()` |
| 2.3 | 🟡 Medium | ⬜ Open | ArtNet receive buffer too small |
| 3.2 | 🟡 Medium | ⬜ Open | ArtNet same-config check uses string join |
| 3.3 | 🟡 Medium | ⬜ Open | LTC audio callback redundant atomic loads |
| 3.4 | 🟡 Medium | ⬜ Open | Audio device scan called from non-main thread |
| 3.5 | 🟡 Medium | ⬜ Open | ArtNet/MTC output timer fires at 1 ms (too hot) |
| 4.1 | 🟡 Medium | ⬜ Open | `MainContentComponent` mega-class (75+ members) |
| 4.2 | 🟡 Medium | ⬜ Open | Vendor engine classes are header-only |
| 4.3 | 🟡 Medium | ⬜ Open | Types in global namespace |
| 4.4 | 🟢 Low | ⬜ Open | `ClockState` unused by engine layer |
| 5.1 | 🟡 Medium | ⬜ Open | `ResolumeClipCollector` missing from CMakeLists |
| 5.2 | 🟢 Low | ⬜ Open | Engine headers not listed in CMakeLists |
| 5.3 | 🟢 Low | ⬜ Open | `#pragma comment(lib)` should be in CMake |
| 5.4 | 🟢 Low | ⬜ Open | ASIO SDK detection overly strict |
| 6.1 | 🟢 Low | ⬜ Open | `addrLower` computed unconditionally |
| 6.2 | 🟢 Low | ⬜ Open | Unused `width`/`height` in `drawPopupMenuBackground` |
| 6.3 | 🟢 Low | ⬜ Open | BOM characters in vendor headers |
| 6.4 | 🟢 Low | ⬜ Open | README describes wrong product |
