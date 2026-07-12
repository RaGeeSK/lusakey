#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

// Read-only Qt list model for the "Authenticator Codes" grid (TotpView.qml).
// Two different refresh rhythms feed it (see AppController):
//   - setRows(): a full reset, for when the SET of TOTP-enabled entries
//     changes (add/update/remove/unlink/unlock/lock).
//   - updateTiming(): an in-place update of just code/secondsRemaining for
//     the SAME entries, called every second — avoids a full model reset
//     (which would flicker the grid) for what's just a countdown/code
//     refresh.
class TotpListModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        qulonglong entryId = 0;
        QString title;
        QString code;
        int secondsRemaining = 0;
    };

    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        TitleRole,
        CodeRole,
        SecondsRemainingRole,
    };

    explicit TotpListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(std::vector<Row> rows);
    // Matches incoming rows to existing ones by entryId (not position) so a
    // stale/reordered caller can't silently corrupt the grid; entries not
    // found in the current set are ignored (a real add/remove should have
    // gone through setRows() instead).
    void updateTiming(const std::vector<Row>& rows);

    const std::vector<Row>& rows() const { return rows_; }

private:
    std::vector<Row> rows_;
};
