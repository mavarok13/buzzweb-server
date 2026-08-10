#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"

#include <optional>
#include <stdexcept>
#include <string>

namespace buzzweb::app {

class ProtocolDecodeError : public std::invalid_argument {
public:
    ProtocolDecodeError(
        std::optional<RequestMeta> meta,
        std::string code,
        std::optional<std::string> request_id = std::nullopt
    );

    const std::optional<RequestMeta>& GetMeta() const;
    const std::optional<std::string>& GetRequestId() const;

private:
    std::optional<RequestMeta> meta_;
    std::optional<std::string> request_id_;
};

ControlDispatcherRequest DecodeRequest(
    const domain::ParticipantId& participant_id,
    const std::string& message
);
std::string EncodeResponse(const ControlDispatcherResponse& response);
std::string EncodeEvent(const ControlDispatcherEvent& event);
std::string EncodeSignalingEvent(const SignalingResponse& response);
std::string EncodeProtocolError(
    const std::optional<RequestMeta>& meta,
    const std::string& code,
    std::optional<std::string> request_id = std::nullopt
);

} // namespace buzzweb::app
