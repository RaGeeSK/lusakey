#pragma once

#include <chrono>

namespace lusakey::core::util {

// Injectable clock so time-dependent logic (auto-lock idle timers) is
// testable without depending on real wall-clock time. Most of the codebase
// (TOTP) instead takes an explicit `now` parameter defaulting to
// std::chrono::system_clock::now(), which covers the common case without
// needing a virtual call; this interface exists for stateful timers (e.g.
// the app-layer auto-lock/clipboard-clear timers in M5) that need to be
// swapped out for a fake clock in tests.
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::chrono::steady_clock::time_point now() const { return std::chrono::steady_clock::now(); }
};

} // namespace lusakey::core::util
