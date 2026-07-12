#pragma once

#include <QAbstractListModel>

#include <vector>

#include "lusakey/core/vault/entry.h"

// Read-only Qt list model populated from a snapshot of
// VaultService::listEntries(). Rebuilt wholesale via setEntries() (called by
// AppController after any mutation or search-text change) — simplest-correct
// approach for the list sizes a personal password manager targets; no
// incremental insert/remove diffing.
class VaultListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        TitleRole,
        UsernameRole,
        HasTotpRole,
        FolderIdRole,
    };

    explicit VaultListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(std::vector<lusakey::core::vault::EntrySummary> entries);

private:
    std::vector<lusakey::core::vault::EntrySummary> entries_;
};
