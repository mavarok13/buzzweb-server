#include "buzzweb/net/Session.hpp"

#include "buzzweb/app/ProtocolCodec.hpp"
#include "buzzweb/net/SessionRegistry.hpp"

#include <boost/asio/ssl/stream_base.hpp>
#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <type_traits>
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
    std::optional<app::ControlDispatcherRequest> request;
    try {
        request = app::DecodeRequest(participant_id_, message);
    } catch (const nlohmann::json::parse_error&) {
        Send(app::EncodeProtocolError(std::nullopt, "invalid_json"));
        return;
    } catch (const app::ProtocolDecodeError& ex) {
        Send(app::EncodeProtocolError(ex.GetMeta(), ex.what(), ex.GetRequestId()));
        return;
    } catch (const std::invalid_argument& ex) {
        Send(app::EncodeProtocolError(std::nullopt, ex.what()));
        return;
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Protocol decode failed: " << ex.what();
        Send(app::EncodeProtocolError(std::nullopt, "invalid_message"));
        return;
    }

    auto self = shared_from_this();
    dispatcher_.Dispatch(*request, [self](const app::ControlDispatcherResult& result) {
        if (const auto* success = std::get_if<app::SuccessResponse>(&result.response)) {
            if (const auto* signaling = std::get_if<app::SignalingResponse>(&success->payload)) {
                auto target = self->registry_.Find(signaling->target_participant_id);
                if (!target) {
                    self->Send(app::EncodeResponse(app::ErrorResponse{
                        success->meta,
                        app::ParticipantUnavailableResponse{}
                    }));
                    return;
                }
                (*target)->Send(app::EncodeSignalingEvent(*signaling));
            }
        }

        self->Send(app::EncodeResponse(result.response));
        self->DeliverEvents(result.events);
    });
}

void SessionBase::DeliverEvents(const std::vector<app::ControlDispatcherEvent>& events)
{
    for (const auto& event : events) {
        const auto encoded = app::EncodeEvent(event);
        std::visit([&](const auto& typed_event) {
            using Event = std::decay_t<decltype(typed_event)>;
            for (const auto& participant : typed_event.event_receivers) {
                if constexpr (std::is_same_v<Event, app::JoinedRoomEvent>) {
                    if (participant.GetId() == typed_event.joined_participant.GetId()) {
                        continue;
                    }
                }

                auto session = registry_.Find(participant.GetId());
                if (session) {
                    (*session)->Send(encoded);
                }
            }
        }, event);
    }
}

std::string SessionBase::InvalidMessageError() const
{
    return app::EncodeProtocolError(std::nullopt, "invalid_message");
}

void SessionBase::RemoveFromRegistry()
{
    if (!registered_) {
        return;
    }

    registered_ = false;
    registry_.Remove(participant_id_);

    try {
        DeliverEvents(dispatcher_.HandleParticipantDisconnected(participant_id_));
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Session " << participant_id_ << " cleanup failed: " << ex.what();
    }
}

PlainSession::PlainSession(
    TcpSocket socket,
    app::ControlDispatcher& dispatcher,
    SessionRegistry& registry
)
    : Session(PlainWebSocketStream(std::move(socket)), dispatcher, registry)
{
}

void PlainSession::Start()
{
    AcceptWebSocket();
}

void PlainSession::CloseLowestLayer(boost::system::error_code& error)
{
    websocket_.next_layer().close(error);
}

SslSession::SslSession(
    TcpSocket socket,
    boost::asio::ssl::context& tls_context,
    app::ControlDispatcher& dispatcher,
    SessionRegistry& registry
)
    : Session(SslWebSocketStream(std::move(socket), tls_context), dispatcher, registry)
{
}

void SslSession::Start()
{
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

void SslSession::CloseLowestLayer(boost::system::error_code& error)
{
    websocket_.next_layer().next_layer().close(error);
}

} // namespace buzzweb::net
