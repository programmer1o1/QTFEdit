#include "MainWindow.h"
#include "CrashReporter.h"

#include <VTFLib.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QObject>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

namespace {

class FileOpenEventFilter final : public QObject {
public:
    explicit FileOpenEventFilter(MainWindow *window) : window_(window) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if(event->type() == QEvent::FileOpen && window_) {
            auto *foe = static_cast<QFileOpenEvent *>(event);
            const QString path = foe->file();
            if(!path.isEmpty()) {
                QTimer::singleShot(0, window_, [w = window_, path] { w->openPath(path); });
            }
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    MainWindow *window_ = nullptr;
};

} // namespace

int main(int argc, char **argv) {
    // Handle --version / --help before we spin up QApplication so the binary can be smoke-tested
    // on headless CI without initializing Qt platform plugins.
    for(int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if(arg == QStringLiteral("--version") || arg == QStringLiteral("-v")) {
#ifndef QTFEDIT_VERSION
#define QTFEDIT_VERSION "unknown"
#endif
            std::fprintf(stdout, "QTFEdit %s (VTFLib %s)\n", QTFEDIT_VERSION, VL_VERSION_STRING);
            return 0;
        }
        if(arg == QStringLiteral("--help") || arg == QStringLiteral("-h")) {
            std::fprintf(stdout,
                "Usage: vtfeditqt [--version|-v] [--help|-h] [path-to-open]\n"
                "  Cross-platform Qt GUI for Valve Texture Format (.vtf) and Material (.vmt) files.\n"
                "  Launch with no args to open the editor. Pass a .vtf, .vmt, or image path to open it on launch.\n");
            return 0;
        }
    }

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VTFEditReloaded");
    QCoreApplication::setApplicationName("VTFEditQt");

    installCrashHandler();

    if(!vlInitialize()) {
        QMessageBox::critical(nullptr, "VTFLib init failed", QString::fromUtf8(vlGetLastError()));
        return 1;
    }

    MainWindow window;
    window.show();

    FileOpenEventFilter fileOpenFilter(&window);
    app.installEventFilter(&fileOpenFilter);

    const QStringList args = QCoreApplication::arguments();
    if(args.size() >= 2) {
        const QString path = args.at(1);
        // Defer opening until the event loop starts so the main window is fully realized.
        QTimer::singleShot(0, &window, [w = &window, path] { w->openPath(path); });
    }

    const int code = app.exec();
    vlShutdown();
    return code;
}
