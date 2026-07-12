#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "lusakey/core/vault/entry.h"

namespace lusakey::core::vault {

struct Folder {
    FolderId id = 0;
    std::string name;
};

// In-memory decrypted vault contents, plus (de)serialization to/from the
// vault file's encrypted payload bytes.
//
// The wire format is a small, dependency-free, hand-rolled binary encoding
// (versioned, length-prefixed fields) rather than a general serialization
// library (e.g. CBOR): the payload is opaque once encrypted, so there is no
// external interoperability requirement to justify a heavier dependency.
class VaultModel {
public:
    VaultModel() = default;

    // Assigns entry.id if it is 0 (new entry); otherwise overwrites any
    // existing entry with that id (used when reconstructing from storage).
    EntryId addEntry(Entry entry);
    // Returns false if no entry with entry.id exists.
    bool updateEntry(const Entry& entry);
    bool removeEntry(EntryId id);

    FolderId addFolder(Folder folder);
    bool removeFolder(FolderId id);
    // Returns false if no folder with this id exists.
    bool renameFolder(FolderId id, const std::string& name);

    const Entry* findEntry(EntryId id) const;
    const std::unordered_map<EntryId, Entry>& entries() const { return entries_; }
    const std::vector<Folder>& folders() const { return folders_; }

    std::vector<std::uint8_t> serialize() const;
    static VaultModel deserialize(const std::vector<std::uint8_t>& bytes);

private:
    std::unordered_map<EntryId, Entry> entries_;
    std::vector<Folder> folders_;
    EntryId nextEntryId_ = 1;
    FolderId nextFolderId_ = 1;
};

} // namespace lusakey::core::vault
