#include "vault_list_model.h"

using lusakey::core::vault::EntrySummary;

VaultListModel::VaultListModel(QObject* parent) : QAbstractListModel(parent) {}

int VaultListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(entries_.size());
}

QVariant VaultListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= entries_.size()) {
        return {};
    }
    const auto& entry = entries_[static_cast<std::size_t>(index.row())];
    switch (role) {
        case EntryIdRole:
            return QVariant::fromValue(static_cast<qulonglong>(entry.id));
        case TitleRole:
            return QString::fromStdString(entry.title);
        case UsernameRole:
            return QString::fromStdString(entry.username);
        case HasTotpRole:
            return entry.hasTotp;
        default:
            return {};
    }
}

QHash<int, QByteArray> VaultListModel::roleNames() const {
    return {
        {EntryIdRole, "entryId"},
        {TitleRole, "title"},
        {UsernameRole, "username"},
        {HasTotpRole, "hasTotp"},
    };
}

void VaultListModel::setEntries(std::vector<EntrySummary> entries) {
    beginResetModel();
    entries_ = std::move(entries);
    endResetModel();
}
