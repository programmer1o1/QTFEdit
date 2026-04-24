#pragma once

#include <QDialog>
#include <QList>
#include <QString>

class QAction;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

class CommandPaletteDialog final : public QDialog {
    Q_OBJECT

public:
    explicit CommandPaletteDialog(const QList<QAction *> &actions, QWidget *parent = nullptr);
    ~CommandPaletteDialog() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onFilterChanged(const QString &text);
    void onItemActivated(QListWidgetItem *item);

private:
    struct Entry {
        QString label;
        QString shortcut;
        QString searchKey;
        QAction *action;
        qint64 lastUsedMs = 0;
        int useCount = 0;
        double recencyScore = 0.0;
    };

    void applyFilter(const QString &text);
    void invokeSelected();

    QList<Entry> entries_;
    QLineEdit *filter_ = nullptr;
    QListWidget *list_ = nullptr;
};
