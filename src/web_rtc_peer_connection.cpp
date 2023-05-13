#include "web_rtc_peer_connection.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include "json/json.hpp"

namespace {
    const char* StunServerURL = "stun:stun.l.google.com:19302";
    const char* SignallingServiceURL = "https://australia-southeast1-comms-link.cloudfunctions.net";

    constexpr std::chrono::minutes MaximumPollingDuration(30);
}

using json = nlohmann::json;

namespace Comms {
    WebRTCPeerConnection::WebRTCPeerConnection(std::string name, std::string password) :
        _rtcConfig(),
        _localSDP(""),
        _name(name),
        _password(password) {
        rtc::InitLogger(rtc::LogLevel::Debug);

        _rtcConfig.iceServers.emplace_back(StunServerURL);

        _peerConnection = std::make_unique<rtc::PeerConnection>(_rtcConfig);
        _peerConnection->onGatheringStateChange([&](rtc::PeerConnection::GatheringState state) {

            if (state == rtc::PeerConnection::GatheringState::Complete) {
                // ICE candidates have been gathered, the local SDP can be made.
                auto description = _peerConnection->localDescription();
                {
                    std::lock_guard<std::mutex> lock(_localSDPMutex);
                    _localSDP = std::string(description.value()); // An offer or answer depending on whether a remote SDP has been set.
                }
                _localSDPNotEmptyCondition.notify_all();
            }
        });
    }

    void WebRTCPeerConnection::Connect()
    {
        auto existingOffer = RetrieveOffer();

        // No existing offer, publish a new one.
        if (std::holds_alternative<std::monostate>(existingOffer)) { 

            GenerateOfferSDP();
            PublishSDP(SDPType::Offer);

            auto state = _peerConnection->state();

            auto answer = RetrieveAnswer();

            if (answer.has_value()) {
                AcceptRemoteSDP(*answer);
            }
        }
        // Offer exists, accept and publish an answer.
        else if (std::holds_alternative<std::string>(existingOffer)) {
            AcceptRemoteSDP(std::get<std::string>(existingOffer));
            PublishSDP(SDPType::Answer);
        }
        // Incorrect password, close the connection.
        else {
            _peerConnection->close();
        }
    }

    rtc::PeerConnection::State WebRTCPeerConnection::GetConnectionState()
    {
        return _peerConnection->state();
    }

    void WebRTCPeerConnection::GenerateOfferSDP() {

        rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
        media.addOpusCodec(96);
        media.setBitrate(256);
        auto track = _peerConnection->addTrack(media);

        _peerConnection->setLocalDescription();

        WaitForLocalSDP(); // Waits for the complete Offer SDP including ICE candidates
    }

    void WebRTCPeerConnection::PublishSDP(const SDPType type) const
    {
        const auto typeString = type == SDPType::Offer ? "offer" : "answer";
        const auto pathName = type == SDPType::Offer ? "/connectionOffer" : "/connectionAnswer";

        json httpBody = {
            {"connectionName", _name},
            {"password", _password},
            {typeString, _localSDP}
        };

        httplib::Client httpClient(SignallingServiceURL);
        httpClient.Post(pathName, httpBody.dump(), "application/json");
    }

    std::variant<std::monostate, bool, std::string> WebRTCPeerConnection::RetrieveOffer() const
    {
        httplib::Client httpClient(SignallingServiceURL);

        httplib::Params httpParams = {
            {"connectionName", _name},
            {"password",_password}
        };
        httplib::Headers httpHeaders{};

        auto response = httpClient.Get("/getOffer", httpParams, httpHeaders);

        if (response->status == 200) {
            return json::parse(response->body).value("data", "");
        }
        else if (response->status == 403) {
            return false;
        }

        return std::monostate();
    }

    std::optional<std::string> WebRTCPeerConnection::RetrieveAnswer() const
    {
        httplib::Client httpClient(SignallingServiceURL);

        httplib::Params httpParams = {
            {"connectionName", _name}
        };
        httplib::Headers httpHeaders{};

        std::chrono::seconds pollingDuration(std::chrono::seconds::zero());
        std::chrono::seconds pollingInterval(1);

        do {
            auto response = httpClient.Get("/getAnswer", httpParams, httpHeaders);

            if (response->status == 200) {
                return json::parse(response->body).value("data", "");
            }
            else if (response->status == 404) {

                if (pollingDuration >= std::chrono::minutes(5)) {
                    pollingInterval = std::chrono::seconds(30);
                }
                else if (pollingDuration >= std::chrono::seconds(30)) {
                    pollingInterval = std::chrono::seconds(5);
                }

                pollingDuration += pollingInterval;
                std::this_thread::sleep_for(pollingInterval);
            }
        } while (pollingDuration < MaximumPollingDuration);

        return std::nullopt;
    }

    void WebRTCPeerConnection::AcceptRemoteSDP(std::string sdp) {
        rtc::Description remoteSDP(sdp);
        _peerConnection->setRemoteDescription(sdp);

        WaitForLocalSDP(); // Waits for the complete Answer SDP including ICE candidates
    }

    void WebRTCPeerConnection::WaitForLocalSDP()
    {
        std::unique_lock<std::mutex> lock(_localSDPMutex);
        _localSDPNotEmptyCondition.wait(lock, [this]() { return !_localSDP.empty(); });
    }
}
