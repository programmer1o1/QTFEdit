#include "Toast.h"

#include <QEnterEvent>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>

Toast::Toast(QWidget *parent) : QFrame(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(true);

    label_ = new QLabel(this);
    label_->setWordWrap(true);
    label_->setTextInteractionFlags(Qt::NoTextInteraction);
    label_->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(12, 8, 12, 8);
    h->addWidget(label_);

    hideTimer_ = new QTimer(this);
    hideTimer_->setSingleShot(true);
    connect(hideTimer_, &QTimer::timeout, this, &QWidget::hide);

    hide();

    if(parent) parent->installEventFilter(this);
}

Toast::~Toast() = default;

void Toast::applyStyle(Level level, bool clickable) {
    QString bg;
    QString fg = "#ffffff";
    QString border;
    switch(level) {
        case Level::Info:    bg = "#2b5279"; border = "#6fa8dc"; break;
        case Level::Warning: bg = "#6e5422"; border = "#f5c048"; break;
        case Level::Error:   bg = "#7a2a2a"; border = "#ef6c6c"; break;
    }

    const QString textDeco = clickable ? "text-decoration: underline;" : "";
    setStyleSheet(QString(
        "QFrame { background-color: %1; border-left: 4px solid %2; border-radius: 4px; }"
        "QLabel { color: %3; font-size: 12px; background: transparent; %4 }"
    ).arg(bg, border, fg, textDeco));
    currentLevel_ = level;
}

void Toast::notify(const QString &text, Level level, int durationMs) {
    clickAction_ = nullptr;
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setCursor(Qt::ArrowCursor);
    applyStyle(level, /*clickable=*/false);

    label_->setText(text);
    label_->adjustSize();
    adjustSize();

    reposition();
    show();
    raise();

    hideTimer_->start(std::max(500, durationMs));
}

void Toast::notifyWithAction(const QString &text, Level level, int durationMs,
                             std::function<void()> onClick) {
    clickAction_ = std::move(onClick);
    setAttribute(Qt::WA_TransparentForMouseEvents, clickAction_ ? false : true);
    setCursor(clickAction_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
    applyStyle(level, /*clickable=*/static_cast<bool>(clickAction_));

    label_->setText(text);
    label_->adjustSize();
    adjustSize();

    reposition();
    show();
    raise();

    hideTimer_->start(std::max(500, durationMs));
}

void Toast::mouseReleaseEvent(QMouseEvent *event) {
    if(clickAction_) {
        auto cb = clickAction_;
        clickAction_ = nullptr;
        hide();
        cb();
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void Toast::enterEvent(QEnterEvent *event) {
    if(clickAction_ && hideTimer_->isActive()) {
        // Pause auto-hide while the user is pointing at the toast.
        hideTimer_->stop();
    }
    QFrame::enterEvent(event);
}

void Toast::leaveEvent(QEvent *event) {
    if(clickAction_) {
        // Resume a short auto-hide after the pointer leaves.
        hideTimer_->start(1500);
    }
    QFrame::leaveEvent(event);
}

bool Toast::eventFilter(QObject *watched, QEvent *event) {
    if(watched == parent() && event->type() == QEvent::Resize) {
        if(isVisible()) reposition();
    }
    return QFrame::eventFilter(watched, event);
}

void Toast::reposition() {
    auto *p = qobject_cast<QWidget *>(parent());
    if(!p) return;
    const int maxW = std::max(200, p->width() - 2 * marginPx_);
    setMaximumWidth(maxW);
    adjustSize();
    const int x = p->width() - width() - marginPx_;
    const int y = p->height() - height() - marginPx_ - 32; // leave room for status bar
    move(std::max(marginPx_, x), std::max(marginPx_, y));
}
