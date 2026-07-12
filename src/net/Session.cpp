#include "buzzweb/net/Session.hpp"

#include "buzzweb/net/SessionRegistry.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/log/trivial.hpp>
#include <cstdint>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace buzzweb::net {
namespace {

domain::ParticipantId GenerateParticipantId()
{
    static thread_local std::mt19937_64 generator(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> distribution;

    std::ostringstream stream;
    stream << "p_" << std::hex << std::setw(16) << std::setfill('0') << distribution(generator);
    return stream.str();
}

std::optional<app::ControlMessageType> MessageTypeFromString(const std::string& type)
{
    if (type == "create_room") {
        return app::ControlMessageType::CreateRoom;
    }
    if (type == "join_room") {
        return app::ControlMessageType::JoinRoom;
    }
    if (type == "leave_room") {
        return app::ControlMessageType::LeaveRoom;
    }
    if (type == "get_room_participants") {
        return app::ControlMessageType::GetRoomParticipants;
    }
    if (type == "offer") {
        return app::ControlMessageType::Offer;
    }
    if (type == "answer") {
        return app::ControlMessageType::Answer;
    }
    if (type == "ice_candidate") {
        return app::ControlMessageType::IceCandidate;
    }

    return std::nullopt;
}

std::string ResultType(app::ControlMessageType type)
{
    switch (type) {
    case app::ControlMessageType::CreateRoom:
        return "create_room_result";
    case app::ControlMessageType::JoinRoom:
        return "join_room_result";
    case app::ControlMessageType::LeaveRoom:
        return "leave_room_result";
    case app::ControlMessageType::GetRoomParticipants:
        return "get_room_participants_result";
    case app::ControlMessageType::Offer:
        return "offer_result";
    case app::ControlMessageType::Answer:
        return "answer_result";
    case app::ControlMessageType::IceCandidate:
        return "ice_candidate_result";
    }

    return "error";
}

std::string RelayType(app::ControlMessageType type) {
    switch (type) {
    case app::ControlMessageType::Offer:
        return "offer";
    case app::ControlMessageType::Answer:
        return "answer";
    case app::ControlMessageType::IceCandidate:
        return "ice_candidate";
    }

    return "error";
}

std::string ErrorMessage(const std::string& code)
{
    if (code == "invalid_json") {
        return "Message must be valid JSON.";
    }
    if (code == "invalid_message") {
        return "Message type or payload is invalid.";
    }

    return "Internal server error.";
}

std::string SerializeError(
    std::string type,
    std::optional<std::string> request_id,
    std::string code
)
{
    nlohmann::json json = {
        {"type", std::move(type)},
        {"ok", false},
        {"error", {
            {"code", code},
            {"message", ErrorMessage(code)}
        }}
    };

    if (request_id.has_value()) {
        json["request_id"] = *request_id;
    }

    return json.dump();
}

std::string SerializeResponse(const app::ControlResponse& response)
{
    nlohmann::json json = {
        {"type", ResultType(response.type)},
        {"ok", response.ok}
    };

    if (response.request_id.has_value()) {
        json["request_id"] = *response.request_id;
    }

    if (response.ok) {
        json["payload"] = response.payload;
    } else if (response.error.has_value()) {
        json["error"] = {
            {"code", response.error->code},
            {"message", response.error->message}
        };
    } else {
        json["error"] = {
            {"code", "internal_error"},
            {"message", "Internal server error."}
        };
    }

    return json.dump();
}

std::string SerializeRelay(const app::ControlRelay& relay) {
    nlohmann::json json = {
        {"type", RelayType(relay.type)},
    };
    json["payload"] = relay.payload;
    json["payload"].update(
        {
            {"target_participant_id", relay.target_participant_id},
            {"from_participant_id", relay.from_participant_id},
            {"room_code", relay.room_code}
        }
    );

    return json.dump();
}

std::optional<std::string> OptionalRequestId(const nlohmann::json& json)
{
    if (!json.contains("request_id") || json.at("request_id").is_null()) {
        return std::nullopt;
    }

    if (!json.at("request_id").is_string()) {
        throw std::invalid_argument("invalid_message");
    }

    return json.at("request_id").get<std::string>();
}

} // namespace

SessionBase::SessionBase(app::ControlDispatcher& dispatcher, SessionRegistry& registry)
    : dispatcher_(dispatcher),
      registry_(registry),
      participant_id_(GenerateParticipantId())
{
}

const domain::ParticipantId& SessionBase::GetParticipantId() const
{
    return participant_id_;
}

void SessionBase::HandleMessage(std::string message)
{
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(message);
    } catch (const nlohmann::json::parse_error&) {
        Send(SerializeError("error", std::nullopt, "invalid_json"));
        return;
    }

    try {
        if (!json.is_object() || !json.contains("type") || !json.at("type").is_string()) {
            Send(SerializeError("error", std::nullopt, "invalid_message"));
            return;
        }

        auto request_id = OptionalRequestId(json);
        const auto type_name = json.at("type").get<std::string>();
        auto type = MessageTypeFromString(type_name);
        if (!type.has_value()) {
            Send(SerializeError("error", std::move(request_id), "invalid_message"));
            return;
        }

        app::ControlMessage control_message{
            *type,
            std::move(request_id),
            json.contains("payload") ? json.at("payload") : nlohmann::json::object()
        };

        auto self = shared_from_this();
        dispatcher_.Dispatch(
            participant_id_,
            control_message,
            [self](const app::ControlResponse& response) {
                self->Send(SerializeResponse(response));
            },
            [self](const app::ControlRelay& relay) {
                auto target_session = self->registry_.Find(relay.target_participant_id);
                if (!target_session) {
                    return false;
                }

                (*target_session)->Send(SerializeRelay(relay));
                return true;
            }
        );
    } catch (const std::exception&) {
        Send(SerializeError("error", std::nullopt, "invalid_message"));
    }
}

std::string SessionBase::InvalidMessageError() const
{
    return SerializeError("error", std::nullopt, "invalid_message");
}

void SessionBase::RemoveFromRegistry()
{
    if (!registered_) {
        return;
    }

    registered_ = false;
    registry_.Remove(participant_id_);

    try {
        dispatcher_.HandleParticipantDisconnected(participant_id_);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Session " << participant_id_ << " failed: " << ex.what();
    }
}

PlainSession::PlainSession(TcpSocket socket, app::ControlDispatcher& dispatcher, SessionRegistry& registry)
: Session(PlainWebSocketStream(std::move(socket)), dispatcher, registry) {
}

void PlainSession::Start() {
    AcceptWebSocket();
}

void PlainSession::CloseLowestLayer(boost::system::error_code& error) {
    websocket_.next_layer().close(error);
}

SslSession::SslSession(TcpSocket socket, boost::asio::ssl::context& tls_context, app::ControlDispatcher& dispatcher, SessionRegistry& registry)
: Session(SslWebSocketStream(std::move(socket), tls_context), dispatcher, registry) {}

void SslSession::Start() {
    auto self = std::static_pointer_cast<SslSession>(shared_from_this());
    websocket_.next_layer().async_handshake(
        boost::asio::ssl::stream_base::server,
        [self](boost::system::error_code error) {
            self->OnTlsHandshake(error);
        }
    );
}

void SslSession::OnTlsHandshake(boost::system::error_code error)
{
    if (error) {
        Fail(error);
        return;
    }

    AcceptWebSocket();
}

void SslSession::CloseLowestLayer(boost::system::error_code& error) {
    websocket_.next_layer().next_layer().close(error);
}

} // namespace buzzweb::net
