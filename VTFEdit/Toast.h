#pragma once

#include <QFrame>
#include <QString>

#include <functional>

class QLabel;
class QPropertyAnimation;
class QTimer;

class Toast final : public QFrame {
    Q_OBJECT

public:
    enum class Level { Info, Warning, Error };

    explicit Toast(QWidget *parent);
    ~Toast() override;

    void notify(const QString &text, Level level = Level::Info, int durationMs = 3500);
    // Same as notify(), but makes the toast clickable. The callback fires on mouse release.
    void notifyWithAction(const QString &text, Level level, int durationMs,
                          std::function<void()> onClick);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void reposition();
    void applyStyle(Level level, bool clickable);

    QLabel *label_ = nullptr;
    QTimer *hideTimer_ = nullptr;
    int marginPx_ = 18;
    std::function<void()> clickAction_;
    Level currentLevel_ = Level::Info;
};
