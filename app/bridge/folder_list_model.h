#pragma once

#include <QAbstractListModel>

#include <vector>

#include "lusakey/core/vault/vault_model.h"

// Read-only Qt list model populated from a snapshot of
// VaultService::listFolders(). Rebuilt wholesale via setFolders() (called by
// AppController after any mutation) — same simplest-correct, no-diffing
// approach as VaultListModel.
class FolderListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        FolderIdRole = Qt::UserRole + 1,
        NameRole,
    };

    explicit FolderListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFolders(std::vector<lusakey::core::vault::Folder> folders);

private:
    std::vector<lusakey::core::vault::Folder> folders_;
};
