#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lusakey/core/totp/totp.h"

namespace lusakey::core::vault {

using EntryId = std::uint64_t;
using FolderId = std::uint64_t;

struct TotpSpec {
    std::vector<std::uint8_t> secret; // raw decoded bytes (NOT Base32 text)
    totp::TotpParams params;
};

// Deliberate, documented trade-off: fields are plain std::string rather than
// crypto::SecureBuffer. A fully SecureBuffer-backed entry model (custom
// container semantics, no std::string ergonomics for CRUD/serialization)
// was judged too complex to get right without a compiler/test loop in the
// session this was first written; SecureBuffer is reserved for the vault's
// derived key, which is the single highest-value secret (it decrypts
// everything at once). Revisit if this ever needs hardening further.
struct Entry {
    EntryId id = 0;
    std::string title;
    std::string username;
    std::string password;
    std::string url;
    std::string notes;
    std::vector<std::string> tags;
    std::optional<FolderId> folderId;
    std::optional<TotpSpec> totp;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
};

// Fields an entry is created/updated from. No id/timestamps — VaultService
// assigns those.
struct EntryDraft {
    std::string title;
    std::string username;
    std::string password;
    std::string url;
    std::string notes;
    std::vector<std::string> tags;
    std::optional<FolderId> folderId;
    std::optional<TotpSpec> totp;
};

// Lightweight projection for list views — avoids materializing full entries
// (passwords/TOTP secrets) just to render a scrollable list.
struct EntrySummary {
    EntryId id = 0;
    std::string title;
    std::string username;
    bool hasTotp = false;
};

} // namespace lusakey::core::vault
