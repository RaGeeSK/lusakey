#include "lusakey/core/vault/vault_model.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace lusakey::core::vault {

namespace {

constexpr std::uint32_t kModelFormatVersion = 1;
constexpr std::size_t kMaxFieldBytes = 1'000'000;         // defense-in-depth cap, not a real-world limit
constexpr std::uint32_t kMaxCollectionCount = 1'000'000;  // ditto

class Writer {
public:
    void u8(std::uint8_t v) { out_.push_back(v); }

    void u32(std::uint32_t v) {
        out_.push_back(static_cast<std::uint8_t>(v & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
    }

    void u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            out_.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
        }
    }

    void bytes(const std::vector<std::uint8_t>& b) {
        u32(static_cast<std::uint32_t>(b.size()));
        out_.insert(out_.end(), b.begin(), b.end());
    }

    void str(const std::string& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        out_.insert(out_.end(), s.begin(), s.end());
    }

    std::vector<std::uint8_t> take() { return std::move(out_); }

private:
    std::vector<std::uint8_t> out_;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& data) : data_(data) {}

    std::uint8_t u8() {
        require(1);
        return data_[pos_++];
    }

    std::uint32_t u32() {
        require(4);
        const std::uint32_t v = static_cast<std::uint32_t>(data_[pos_]) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 1]) << 8) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 2]) << 16) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return v;
    }

    std::uint64_t u64() {
        require(8);
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v |= static_cast<std::uint64_t>(data_[pos_ + static_cast<std::size_t>(i)]) << (i * 8);
        }
        pos_ += 8;
        return v;
    }

    std::vector<std::uint8_t> bytes() {
        const auto len = u32();
        if (len > kMaxFieldBytes) {
            throw std::runtime_error("VaultModel: field length exceeds sanity cap");
        }
        require(len);
        std::vector<std::uint8_t> out(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
                                       data_.begin() + static_cast<std::ptrdiff_t>(pos_ + len));
        pos_ += len;
        return out;
    }

    std::string str() {
        const auto raw = bytes();
        return std::string(raw.begin(), raw.end());
    }

private:
    void require(std::size_t n) const {
        if (pos_ + n > data_.size()) {
            throw std::runtime_error("VaultModel: truncated/corrupt payload");
        }
    }

    const std::vector<std::uint8_t>& data_;
    std::size_t pos_ = 0;
};

} // namespace

EntryId VaultModel::addEntry(Entry entry) {
    if (entry.id == 0) {
        entry.id = nextEntryId_++;
    } else {
        nextEntryId_ = std::max(nextEntryId_, entry.id + 1);
    }
    const auto id = entry.id;
    entries_[id] = std::move(entry);
    return id;
}

bool VaultModel::updateEntry(const Entry& entry) {
    const auto it = entries_.find(entry.id);
    if (it == entries_.end()) {
        return false;
    }
    it->second = entry;
    return true;
}

bool VaultModel::removeEntry(EntryId id) {
    return entries_.erase(id) > 0;
}

FolderId VaultModel::addFolder(Folder folder) {
    if (folder.id == 0) {
        folder.id = nextFolderId_++;
    } else {
        nextFolderId_ = std::max(nextFolderId_, folder.id + 1);
    }
    const auto id = folder.id;
    folders_.push_back(Folder{id, folder.name});
    return id;
}

bool VaultModel::removeFolder(FolderId id) {
    const auto before = folders_.size();
    folders_.erase(std::remove_if(folders_.begin(), folders_.end(),
                                   [id](const Folder& f) { return f.id == id; }),
                   folders_.end());
    return folders_.size() != before;
}

const Entry* VaultModel::findEntry(EntryId id) const {
    const auto it = entries_.find(id);
    return it == entries_.end() ? nullptr : &it->second;
}

std::vector<std::uint8_t> VaultModel::serialize() const {
    Writer w;
    w.u32(kModelFormatVersion);
    w.u64(nextEntryId_);
    w.u64(nextFolderId_);

    w.u32(static_cast<std::uint32_t>(folders_.size()));
    for (const auto& folder : folders_) {
        w.u64(folder.id);
        w.str(folder.name);
    }

    w.u32(static_cast<std::uint32_t>(entries_.size()));
    for (const auto& [id, entry] : entries_) {
        w.u64(entry.id);
        w.str(entry.title);
        w.str(entry.username);
        w.str(entry.password);
        w.str(entry.url);
        w.str(entry.notes);

        w.u32(static_cast<std::uint32_t>(entry.tags.size()));
        for (const auto& tag : entry.tags) {
            w.str(tag);
        }

        w.u8(entry.folderId.has_value() ? 1 : 0);
        if (entry.folderId) {
            w.u64(*entry.folderId);
        }

        w.u8(entry.totp.has_value() ? 1 : 0);
        if (entry.totp) {
            w.bytes(entry.totp->secret);
            w.u8(static_cast<std::uint8_t>(entry.totp->params.algorithm));
            w.u32(entry.totp->params.digits);
            w.u32(entry.totp->params.periodSeconds);
        }

        w.u64(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(entry.createdAt.time_since_epoch()).count()));
        w.u64(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(entry.modifiedAt.time_since_epoch()).count()));
    }

    return w.take();
}

VaultModel VaultModel::deserialize(const std::vector<std::uint8_t>& bytes) {
    Reader r(bytes);
    VaultModel model;

    const auto version = r.u32();
    if (version != kModelFormatVersion) {
        throw std::runtime_error("VaultModel: unsupported payload version");
    }

    model.nextEntryId_ = r.u64();
    model.nextFolderId_ = r.u64();

    const auto folderCount = r.u32();
    if (folderCount > kMaxCollectionCount) {
        throw std::runtime_error("VaultModel: folder count exceeds sanity cap");
    }
    model.folders_.reserve(folderCount);
    for (std::uint32_t i = 0; i < folderCount; ++i) {
        Folder folder;
        folder.id = r.u64();
        folder.name = r.str();
        model.folders_.push_back(std::move(folder));
    }

    const auto entryCount = r.u32();
    if (entryCount > kMaxCollectionCount) {
        throw std::runtime_error("VaultModel: entry count exceeds sanity cap");
    }
    for (std::uint32_t i = 0; i < entryCount; ++i) {
        Entry entry;
        entry.id = r.u64();
        entry.title = r.str();
        entry.username = r.str();
        entry.password = r.str();
        entry.url = r.str();
        entry.notes = r.str();

        const auto tagCount = r.u32();
        if (tagCount > kMaxCollectionCount) {
            throw std::runtime_error("VaultModel: tag count exceeds sanity cap");
        }
        entry.tags.reserve(tagCount);
        for (std::uint32_t t = 0; t < tagCount; ++t) {
            entry.tags.push_back(r.str());
        }

        if (r.u8() != 0) {
            entry.folderId = r.u64();
        }

        if (r.u8() != 0) {
            TotpSpec spec;
            spec.secret = r.bytes();
            spec.params.algorithm = static_cast<totp::HashAlgorithm>(r.u8());
            spec.params.digits = r.u32();
            spec.params.periodSeconds = r.u32();
            entry.totp = std::move(spec);
        }

        entry.createdAt =
            std::chrono::system_clock::time_point(std::chrono::seconds(static_cast<std::int64_t>(r.u64())));
        entry.modifiedAt =
            std::chrono::system_clock::time_point(std::chrono::seconds(static_cast<std::int64_t>(r.u64())));

        model.entries_[entry.id] = std::move(entry);
    }

    return model;
}

} // namespace lusakey::core::vault
