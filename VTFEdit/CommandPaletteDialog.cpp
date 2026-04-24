#include "CommandPaletteDialog.h"

#include <QAction>
#include <QDateTime>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {

QString recencyKeyFor(const QString &label) {
    return QStringLiteral("palette/lastUsedMs/") + label;
}

QString useCountKeyFor(const QString &label) {
    return QStringLiteral("palette/useCount/") + label;
}

// Fuzzy subsequence score. Returns -1 if no subsequence match.
// Higher is better. Rewards: start-of-string, word-boundary, consecutive chars.
int fuzzyScore(const QString &needle, const QString &haystackLower) {
    if(needle.isEmpty()) return 0;
    const int n = needle.size();
    const int h = haystackLower.size();
    if(n > h) return -1;

    int score = 0;
    int hi = 0;
    int prevHi = -2;
    for(int ni = 0; ni < n; ++ni) {
        const QChar c = needle.at(ni);
        int found = -1;
        for(int k = hi; k < h; ++k) {
            if(haystackLower.at(k) == c) { found = k; break; }
        }
        if(found < 0) return -1;

        if(found == 0) score += 15;
        else if(haystackLower.at(found - 1).isSpace() || haystackLower.at(found - 1) == QLatin1Char('-')
                || haystackLower.at(found - 1) == QLatin1Char('/')) {
            score += 10;
        }
        if(found == prevHi + 1) score += 8;

        score += 2;
        prevHi = found;
        hi = found + 1;
    }

    // Length penalty — shorter labels slightly preferred so single-word matches win over embedded ones.
    score -= h / 20;
    return score;
}

double recencyBoost(qint64 lastUsedMs, qint64 nowMs) {
    if(lastUsedMs <= 0) return 0.0;
    const double ageMs = static_cast<double>(nowMs - lastUsedMs);
    if(ageMs <= 0.0) return 20.0;
    const double halfLifeMs = 1000.0 * 60 * 60 * 24 * 3; // 3 days
    return 20.0 * std::exp(-ageMs / halfLifeMs);
}

double useCountBoost(int count) {
    if(count <= 0) return 0.0;
    return 6.0 * std::log(1.0 + count);
}

} // namespace

CommandPaletteDialog::CommandPaletteDialog(const QList<QAction *> &actions, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Command Palette");
    setModal(true);
    resize(560, 440);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText("Type to filter commands…");
    filter_->setClearButtonEnabled(true);
    filter_->installEventFilter(this);
    layout->addWidget(filter_);

    list_ = new QListWidget(this);
    list_->setUniformItemSizes(true);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list_, 1);

    QSettings s;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for(QAction *a : actions) {
        if(!a) continue;
        if(a->isSeparator()) continue;
        QString label = a->text();
        if(label.isEmpty()) continue;
        label.remove(QChar('&'));
        if(label.endsWith(QStringLiteral("…"))) label.chop(1);
        if(!a->isEnabled()) continue;

        Entry e;
        e.label = label;
        e.shortcut = a->shortcut().toString(QKeySequence::NativeText);
        e.searchKey = label.toLower();
        e.action = a;
        e.lastUsedMs = s.value(recencyKeyFor(label), qint64(0)).toLongLong();
        e.useCount = s.value(useCountKeyFor(label), 0).toInt();
        e.recencyScore = recencyBoost(e.lastUsedMs, now) + useCountBoost(e.useCount);
        entries_.append(e);
    }

    connect(filter_, &QLineEdit::textChanged, this, &CommandPaletteDialog::onFilterChanged);
    connect(list_, &QListWidget::itemActivated, this, &CommandPaletteDialog::onItemActivated);

    applyFilter(QString());
    if(list_->count() > 0) list_->setCurrentRow(0);
    filter_->setFocus();
}

CommandPaletteDialog::~CommandPaletteDialog() = default;

void CommandPaletteDialog::applyFilter(const QString &text) {
    const QString needle = text.trimmed().toLower();
    struct Ranked { int entryIdx; int score; };
    QList<Ranked> ranked;
    ranked.reserve(entries_.size());

    if(needle.isEmpty()) {
        for(int i = 0; i < entries_.size(); ++i) {
            ranked.push_back({i, static_cast<int>(std::lround(entries_[i].recencyScore * 100.0))});
        }
        std::stable_sort(ranked.begin(), ranked.end(), [&](const Ranked &a, const Ranked &b) {
            if(a.score != b.score) return a.score > b.score;
            return entries_[a.entryIdx].label.compare(entries_[b.entryIdx].label, Qt::CaseInsensitive) < 0;
        });
    } else {
        for(int i = 0; i < entries_.size(); ++i) {
            const int s = fuzzyScore(needle, entries_[i].searchKey);
            if(s < 0) continue;
            const int total = s * 100 + static_cast<int>(std::lround(entries_[i].recencyScore));
            ranked.push_back({i, total});
        }
        std::stable_sort(ranked.begin(), ranked.end(), [&](const Ranked &a, const Ranked &b) {
            return a.score > b.score;
        });
    }

    list_->clear();
    for(const auto &r : ranked) {
        const auto &e = entries_[r.entryIdx];
        const QString display = e.shortcut.isEmpty()
                                    ? e.label
                                    : QString("%1\t%2").arg(e.label, e.shortcut);
        auto *item = new QListWidgetItem(display, list_);
        item->setData(Qt::UserRole, r.entryIdx);
    }
    if(list_->count() > 0) list_->setCurrentRow(0);
}

void CommandPaletteDialog::onFilterChanged(const QString &text) {
    applyFilter(text);
}

void CommandPaletteDialog::onItemActivated(QListWidgetItem * /*item*/) {
    invokeSelected();
}

void CommandPaletteDialog::invokeSelected() {
    auto *item = list_->currentItem();
    if(!item) return;
    const int idx = item->data(Qt::UserRole).toInt();
    if(idx < 0 || idx >= entries_.size()) return;
    QAction *a = entries_[idx].action;
    const QString label = entries_[idx].label;
    QSettings s;
    s.setValue(recencyKeyFor(label), QDateTime::currentMSecsSinceEpoch());
    s.setValue(useCountKeyFor(label), entries_[idx].useCount + 1);
    accept();
    if(a) a->trigger();
}

bool CommandPaletteDialog::eventFilter(QObject *watched, QEvent *event) {
    if(watched == filter_ && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        switch(ke->key()) {
            case Qt::Key_Down: {
                const int next = qMin(list_->currentRow() + 1, list_->count() - 1);
                list_->setCurrentRow(next);
                return true;
            }
            case Qt::Key_Up: {
                const int prev = qMax(list_->currentRow() - 1, 0);
                list_->setCurrentRow(prev);
                return true;
            }
            case Qt::Key_Return:
            case Qt::Key_Enter:
                invokeSelected();
                return true;
            default:
                break;
        }
    }
    return QDialog::eventFilter(watched, event);
}
