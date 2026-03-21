## Easy Bridge 2.7.1

### Updates and UI

- Added built-in update checking on startup and from `Help -> Check for Updates`.
- Added an in-app `Update Available` window with platform-aware update handling.
- Windows update flow now downloads the installer and starts the update automatically after Easy Bridge closes.
- macOS update flow now downloads and opens the latest DMG for replacement.

### Interface and polish

- Continued the UI refactor by separating more layout and window logic out of the main monolithic path.
- Fixed the Windows dark title bar for the update prompt so it matches the rest of the app.
- Updated the help file and README to document the new update flow.

### Packaging and release

- Installer now uses the icon from the shared `Icons` asset pipeline, matching the application icon.
- Release workflow now checks out the repository before publishing, so release notes are attached reliably.
- Cleaned up the GitHub Release body flow to use the project release notes directly.
