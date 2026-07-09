#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

#include "lusakey/ipc/native_message_channel.h"

using namespace lusakey::core::ipc;

TEST_CASE("writeMessage/readMessage round-trips a JSON string", "[ipc]") {
    std::stringstream stream;
    writeMessage(stream, R"({"action":"ping"})");

    const auto message = readMessage(stream);
    REQUIRE(message.has_value());
    REQUIRE(*message == R"({"action":"ping"})");
}

TEST_CASE("readMessage returns nullopt on clean EOF between messages", "[ipc]") {
    std::stringstream stream;
    const auto message = readMessage(stream);
    REQUIRE_FALSE(message.has_value());
}

TEST_CASE("readMessage handles an empty-body message", "[ipc]") {
    std::stringstream stream;
    writeMessage(stream, "");
    const auto message = readMessage(stream);
    REQUIRE(message.has_value());
    REQUIRE(message->empty());
}

TEST_CASE("writeMessage rejects a message larger than the cap", "[ipc]") {
    std::stringstream stream;
    const std::string tooLarge(kMaxMessageBytes + 1, 'x');
    REQUIRE_THROWS_AS(writeMessage(stream, tooLarge), std::runtime_error);
}

TEST_CASE("readMessage reads multiple consecutive framed messages", "[ipc]") {
    std::stringstream stream;
    writeMessage(stream, "first");
    writeMessage(stream, "second");

    const auto first = readMessage(stream);
    const auto second = readMessage(stream);
    const auto third = readMessage(stream);

    REQUIRE(first.value() == "first");
    REQUIRE(second.value() == "second");
    REQUIRE_FALSE(third.has_value());
}
