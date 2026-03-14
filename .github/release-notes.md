## What's fixed

- Fixed Windows audio device scanning so LTC input/output devices are discovered more reliably and much faster.
- Reworked scan flow to match the faster SuperTimecodeConverter pattern: enumerate device names first, query channel counts only for the selected device.
- Reduced rescan instability on Windows by avoiding heavy per-device open attempts during the full scan.
- Improved startup rendering to reduce the white window flash before the UI appears.
- Windows release build now generates the app icon correctly from the PNG asset when ImageMagick is available.
