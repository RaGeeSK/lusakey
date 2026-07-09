#pragma once

#include <cstddef>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

namespace lusakey::core::ipc {

// Chrome/Firefox "native messaging" framing: a 4-byte length prefix
// (encoded/decoded here explicitly as little-endian — every lusakey target
// platform is little-endian, same rationale as the vault file format),
// followed by that many bytes of UTF-8 JSON. This is the ONLY thing this
// library does; JSON parsing/routing is left to whatever links it (the
// future nmhost executable), so this stays a tiny, dependency-free library
// that both the GUI's bridge layer and nmhost could in principle share.
//
// Messages are capped at kMaxMessageBytes (matching Chrome's own 1 MB
// incoming-message limit) so a malformed/hostile length prefix cannot
// trigger an unbounded allocation.
inline constexpr std::size_t kMaxMessageBytes = 1024 * 1024;

// Reads one framed message from `in`. Returns std::nullopt on a clean EOF
// between messages (the browser closed the pipe) — the caller's read loop
// should stop. Throws std::runtime_error if the stream errors mid-message or
// the length prefix exceeds kMaxMessageBytes.
std::optional<std::string> readMessage(std::istream& in);

// Writes one framed message to `out` and flushes it. Throws
// std::runtime_error if `message` exceeds kMaxMessageBytes or the write fails.
void writeMessage(std::ostream& out, const std::string& message);

} // namespace lusakey::core::ipc
