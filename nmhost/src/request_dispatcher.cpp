#include "lusakey/nmhost/request_dispatcher.h"

#include <utility>

#include <nlohmann/json.hpp>

#include "lusakey/core/util/password_generator.h"

using lusakey::core::util::PasswordGeneratorOptions;
using lusakey::core::vault::EntryFilter;
using lusakey::core::vault::EntryId;
using lusakey::core::vault::ServiceError;
using lusakey::core::vault::ServiceException;

namespace lusakey::nmhost {

namespace {

std::string errorCode(ServiceError err) {
    switch (err) {
        case ServiceError::WrongPassword:
            return "WrongPassword";
        case ServiceError::WrongAnswers:
            return "WrongAnswers";
        case ServiceError::RecoveryNotConfigured:
            return "RecoveryNotConfigured";
        case ServiceError::FileCorrupt:
            return "FileCorrupt";
        case ServiceError::FileNotFound:
            return "FileNotFound";
        case ServiceError::NotUnlocked:
            return "NotUnlocked";
        case ServiceError::EntryNotFound:
            return "EntryNotFound";
        case ServiceError::FolderNotFound:
            return "FolderNotFound";
        case ServiceError::IoError:
            return "IoError";
        case ServiceError::InvalidArgument:
            return "InvalidArgument";
    }
    return "Unknown";
}

std::string errorResponse(const std::string& message, const std::string& code) {
    return nlohmann::json{{"ok", false}, {"error", message}, {"code", code}}.dump();
}

std::string okResponse(nlohmann::json result = nlohmann::json::object()) {
    return nlohmann::json{{"ok", true}, {"result", std::move(result)}}.dump();
}

} // namespace

RequestDispatcher::RequestDispatcher(lusakey::core::vault::VaultService& service, std::filesystem::path vaultPath)
    : service_(service), vaultPath_(std::move(vaultPath)) {}

std::string RequestDispatcher::dispatch(const std::string& rawJson) {
    nlohmann::json request;
    try {
        request = nlohmann::json::parse(rawJson);
    } catch (const nlohmann::json::parse_error& e) {
        return errorResponse(std::string("invalid JSON: ") + e.what(), "InvalidJson");
    }

    const std::string action = request.value("action", "");

    try {
        if (action == "ping") {
            return okResponse("pong");
        }
        if (action == "unlock") {
            service_.unlock(vaultPath_, request.at("password").get<std::string>());
            return okResponse();
        }
        if (action == "lock") {
            service_.lock();
            return okResponse();
        }
        if (action == "listEntries") {
            EntryFilter filter;
            if (request.contains("searchText")) {
                filter.searchText = request.at("searchText").get<std::string>();
            }
            auto entries = nlohmann::json::array();
            for (const auto& e : service_.listEntries(filter)) {
                entries.push_back({
                    {"id", e.id},
                    {"title", e.title},
                    {"username", e.username},
                    {"hasTotp", e.hasTotp},
                });
            }
            return okResponse(std::move(entries));
        }
        if (action == "getEntry") {
            const auto entry = service_.getEntry(request.at("id").get<EntryId>());
            return okResponse({
                {"id", entry.id},
                {"title", entry.title},
                {"username", entry.username},
                {"password", entry.password},
                {"url", entry.url},
            });
        }
        if (action == "currentTotpCode") {
            const auto id = request.at("id").get<EntryId>();
            return okResponse({
                {"code", service_.currentTotpCode(id)},
                {"secondsRemaining", service_.totpSecondsRemaining(id)},
            });
        }
        if (action == "generatePassword") {
            PasswordGeneratorOptions options;
            if (request.contains("length")) {
                options.length = request.at("length").get<unsigned int>();
            }
            if (request.contains("includeUppercase")) {
                options.includeUppercase = request.at("includeUppercase").get<bool>();
            }
            if (request.contains("includeLowercase")) {
                options.includeLowercase = request.at("includeLowercase").get<bool>();
            }
            if (request.contains("includeDigits")) {
                options.includeDigits = request.at("includeDigits").get<bool>();
            }
            if (request.contains("includeSymbols")) {
                options.includeSymbols = request.at("includeSymbols").get<bool>();
            }
            if (request.contains("excludeAmbiguous")) {
                options.excludeAmbiguous = request.at("excludeAmbiguous").get<bool>();
            }
            return okResponse({{"password", service_.generatePassword(options)}});
        }
        return errorResponse("unknown action: " + action, "UnknownAction");
    } catch (const ServiceException& e) {
        return errorResponse(e.what(), errorCode(e.code()));
    } catch (const nlohmann::json::exception& e) {
        return errorResponse(std::string("invalid request: ") + e.what(), "InvalidRequest");
    } catch (const std::exception& e) {
        // Catches e.g. generatePassword()'s std::invalid_argument, which
        // VaultService passes through unwrapped rather than as a
        // ServiceException — still must not escape dispatch() as an
        // uncaught exception.
        return errorResponse(e.what(), "InvalidArgument");
    }
}

} // namespace lusakey::nmhost
