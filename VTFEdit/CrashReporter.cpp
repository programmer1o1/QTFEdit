#include "CrashReporter.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(Q_OS_WIN) || defined(_WIN32)
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#else
    #include <csignal>
    #include <execinfo.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace {

// Signal-safe: holds a pre-resolved path to the crash file we will open on crash.
// Sized for ~PATH_MAX; if truncation happens we still fall back to a short, stable path.
static char g_crashPath[4096] = {0};

#if !defined(_WIN32)

static ssize_t safe_write_cstr(int fd, const char *s) {
    size_t n = 0;
    while(s[n] != '\0') ++n;
    return write(fd, s, n);
}

static void safe_write_u64(int fd, unsigned long long v) {
    char buf[32];
    int len = 0;
    if(v == 0) { buf[len++] = '0'; }
    else {
        char tmp[32];
        int t = 0;
        while(v) { tmp[t++] = char('0' + (v % 10)); v /= 10; }
        while(t--) buf[len++] = tmp[t];
    }
    (void)write(fd, buf, (size_t)len);
}

static const char *signal_name(int sig) {
    switch(sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        case SIGBUS:  return "SIGBUS";
        default:      return "UNKNOWN";
    }
}

extern "C" void crash_signal_handler(int sig) {
    const int fd = open(g_crashPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd >= 0) {
        safe_write_cstr(fd, "QTFEdit crash report\n");
        safe_write_cstr(fd, "signal: ");
        safe_write_cstr(fd, signal_name(sig));
        safe_write_cstr(fd, " (");
        safe_write_u64(fd, (unsigned long long)sig);
        safe_write_cstr(fd, ")\n");

    #if defined(__GLIBC__) || defined(__APPLE__)
        void *frames[64];
        const int depth = backtrace(frames, 64);
        safe_write_cstr(fd, "backtrace:\n");
        backtrace_symbols_fd(frames, depth, fd);
    #else
        safe_write_cstr(fd, "backtrace: unavailable on this platform\n");
    #endif

        close(fd);
    }

    // Re-raise with default disposition so the OS still produces its own core / crash dialog.
    struct sigaction dfl;
    std::memset(&dfl, 0, sizeof(dfl));
    dfl.sa_handler = SIG_DFL;
    sigaction(sig, &dfl, nullptr);
    raise(sig);
}

#else  // _WIN32

static LONG WINAPI crash_exception_filter(EXCEPTION_POINTERS *info) {
    HANDLE fh = CreateFileA(g_crashPath, GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(fh != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        char buf[256];
        int n = _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                            "QTFEdit crash report\r\nexception_code: 0x%08lX\r\n",
                            info ? info->ExceptionRecord->ExceptionCode : 0);
        if(n > 0) WriteFile(fh, buf, (DWORD)n, &written, nullptr);

        // Minimal backtrace via CaptureStackBackTrace.
        void *frames[64];
        USHORT depth = CaptureStackBackTrace(0, 64, frames, nullptr);
        n = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "frames: %u\r\n", (unsigned)depth);
        if(n > 0) WriteFile(fh, buf, (DWORD)n, &written, nullptr);
        for(USHORT i = 0; i < depth; ++i) {
            n = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "  0x%p\r\n", frames[i]);
            if(n > 0) WriteFile(fh, buf, (DWORD)n, &written, nullptr);
        }

        CloseHandle(fh);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif // _WIN32

static QString resolveCrashDir() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(base.isEmpty()) base = QDir::tempPath();
    return QDir(base).filePath("crashes");
}

} // namespace

QString crashReportDirectory() {
    return resolveCrashDir();
}

int countUnseenCrashReports() {
    const QString dir = resolveCrashDir();
    QFileInfoList entries = QDir(dir).entryInfoList(
        QStringList{QStringLiteral("crash-*.txt")}, QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    if(entries.isEmpty()) return 0;

    QSettings s;
    const qint64 lastSeen = s.value(QStringLiteral("crashes/lastSeenMs"), qint64(0)).toLongLong();
    int unseen = 0;
    qint64 newest = lastSeen;
    for(const QFileInfo &fi : entries) {
        const qint64 ms = fi.lastModified().toMSecsSinceEpoch();
        if(ms > lastSeen) ++unseen;
        if(ms > newest) newest = ms;
    }
    if(newest > lastSeen) {
        s.setValue(QStringLiteral("crashes/lastSeenMs"), newest);
    }
    return unseen;
}

void installCrashHandler() {
    const QString dir = resolveCrashDir();
    QDir().mkpath(dir);
    const qint64 ms = QDateTime::currentMSecsSinceEpoch();
    const QString path = QDir(dir).filePath(QStringLiteral("crash-%1.txt").arg(ms));
    const QByteArray utf8 = path.toUtf8();
    const size_t n = std::min(sizeof(g_crashPath) - 1, (size_t)utf8.size());
    std::memcpy(g_crashPath, utf8.constData(), n);
    g_crashPath[n] = '\0';

#if !defined(_WIN32)
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crash_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    const int sigs[] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
    for(int s : sigs) {
        sigaction(s, &sa, nullptr);
    }
#else
    SetUnhandledExceptionFilter(crash_exception_filter);
#endif
}
