#include "folder_list_model.h"

using lusakey::core::vault::Folder;

FolderListModel::FolderListModel(QObject* parent) : QAbstractListModel(parent) {}

int FolderListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(folders_.size());
}

QVariant FolderListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= folders_.size()) {
        return {};
    }
    const auto& folder = folders_[static_cast<std::size_t>(index.row())];
    switch (role) {
        case FolderIdRole:
            return QVariant::fromValue(static_cast<qulonglong>(folder.id));
        case NameRole:
            return QString::fromStdString(folder.name);
        default:
            return {};
    }
}

QHash<int, QByteArray> FolderListModel::roleNames() const {
    return {
        {FolderIdRole, "folderId"},
        {NameRole, "name"},
    };
}

void FolderListModel::setFolders(std::vector<Folder> folders) {
    beginResetModel();
    folders_ = std::move(folders);
    endResetModel();
}
