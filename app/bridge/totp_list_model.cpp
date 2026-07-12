#include "totp_list_model.h"

TotpListModel::TotpListModel(QObject* parent) : QAbstractListModel(parent) {}

int TotpListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(rows_.size());
}

QVariant TotpListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= rows_.size()) {
        return {};
    }
    const auto& row = rows_[static_cast<std::size_t>(index.row())];
    switch (role) {
        case EntryIdRole:
            return QVariant::fromValue(row.entryId);
        case TitleRole:
            return row.title;
        case CodeRole:
            return row.code;
        case SecondsRemainingRole:
            return row.secondsRemaining;
        default:
            return {};
    }
}

QHash<int, QByteArray> TotpListModel::roleNames() const {
    return {
        {EntryIdRole, "entryId"},
        {TitleRole, "title"},
        {CodeRole, "code"},
        {SecondsRemainingRole, "secondsRemaining"},
    };
}

void TotpListModel::setRows(std::vector<Row> rows) {
    beginResetModel();
    rows_ = std::move(rows);
    endResetModel();
}

void TotpListModel::updateTiming(const std::vector<Row>& rows) {
    for (const auto& incoming : rows) {
        for (std::size_t i = 0; i < rows_.size(); ++i) {
            if (rows_[i].entryId != incoming.entryId) {
                continue;
            }
            if (rows_[i].code != incoming.code || rows_[i].secondsRemaining != incoming.secondsRemaining) {
                rows_[i].code = incoming.code;
                rows_[i].secondsRemaining = incoming.secondsRemaining;
                const auto idx = index(static_cast<int>(i));
                emit dataChanged(idx, idx, {CodeRole, SecondsRemainingRole});
            }
            break;
        }
    }
}
