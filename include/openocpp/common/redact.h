#ifndef CHARGELAB_OPEN_FIRMWARE_REDACT_H
#define CHARGELAB_OPEN_FIRMWARE_REDACT_H

// ShoreOCPP patch 0003: keep passwords and keys out of the log (the log can be uploaded to the CSMS with
// GetLog, and is read on the serial monitor).

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace chargelab::redact {
    // True if the text mentions something secret: a password, key, token...
    inline bool mentionsSecret(std::string_view text) {
        // (not "token": OCPP's idToken is a driver's ID, not a secret, and is in most transaction messages)
        static constexpr std::string_view kWords[] = {
                "password", "passwd", "passphrase", "secret", "authorizationkey", "privatekey", "psk"
        };
        std::string lower;
        lower.reserve(text.size());
        for (char c : text)
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        for (auto const& w : kWords)
            if (lower.find(w) != std::string::npos)
                return true;
        return false;
    }

    // An OCPP-J message for the log: unchanged, or, if it may carry a secret, only its start
    // ([type, "id", "Action", ...) with the payload hidden.
    inline std::string message(std::string_view text) {
        if (!mentionsSecret(text))
            return std::string{text};
        auto const payload = text.find('{');
        auto const keep = payload == std::string_view::npos ? std::min<std::size_t>(text.size(), 40) : payload;
        return std::string{text.substr(0, keep)} + "... (payload hidden: may contain a password or key)";
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_REDACT_H
