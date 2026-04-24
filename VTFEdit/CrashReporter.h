#pragma once

class QString;

// Install OS-level crash handlers that write a minimal crash report to the app-data directory.
// On POSIX (Linux, macOS) this installs signal handlers for SIGSEGV / SIGABRT / SIGFPE / SIGILL / SIGBUS.
// On Windows it installs an unhandled-exception filter.
//
// Crash reports land in:
//   macOS:   ~/Library/Application Support/VTFEditReloaded/VTFEditQt/crashes/crash-<epochms>.txt
//   Linux:   ~/.local/share/VTFEditReloaded/VTFEditQt/crashes/crash-<epochms>.txt
//   Windows: %APPDATA%/VTFEditReloaded/VTFEditQt/crashes/crash-<epochms>.txt
//
// The handler is strictly async-signal-safe on POSIX: it uses plain open/write/close and
// backtrace_symbols_fd (signal-safe), never Qt or C++ runtime allocation.
//
// Call after QApplication is constructed so QStandardPaths resolves correctly.
void installCrashHandler();

// Returns the directory where reports are written (may not exist yet).
QString crashReportDirectory();

// Scan the crash-report directory for reports newer than the last-seen timestamp
// in QSettings ("crashes/lastSeenMs"). Returns the count and mutates QSettings
// to mark them seen. Returns 0 on first ever call unless reports already exist.
int countUnseenCrashReports();
