#include "web_rtc_peer_connection.h"

#include <thread>
#include <chrono>

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
    WebRTCPeerConnection::WebRTCPeerConnection() :
        _rtcConfig(),
        _localSDP("") {
        rtc::InitLogger(rtc::LogLevel::Debug);

        _rtcConfig.iceServers.emplace_back(StunServerURL);

        _peerConnection = std::make_shared<rtc::PeerConnection>(_rtcConfig);
        _peerConnection->onGatheringStateChange([&](rtc::PeerConnection::GatheringState state) {

            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto description = _peerConnection->localDescription();

                _localSDP = std::string(description.value());
            }
            });
    }

    std::string WebRTCPeerConnection::GetLocalSDP() {
        return _localSDP;
    }

    void WebRTCPeerConnection::GenerateOfferSDP() {

        rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
        media.addOpusCodec(96);
        media.setBitrate(256);
        auto track = _peerConnection->addTrack(media);

        _peerConnection->setLocalDescription();
    }

    std::string WebRTCPeerConnection::PublishSDP(const SDPType type, const std::string& sessionID, const std::string& password = "") const
    {
        const auto typeString = type == SDPType::Offer ? "offer" : "answer";
        const auto pathName = type == SDPType::Offer ? "/sessionOffer" : "/sessionAnswer";

        json httpBody = {
            {"sessionID", sessionID},
            {"password", password},
            {typeString, _localSDP}
        };

        httplib::Client httpClient(SignallingServiceURL);
        auto response = httpClient.Post(pathName, httpBody.dump(), "application/json");

        if (!response) {
            return "Error publishing offer: no response.";
        }
        else if (response->status == 200) {
            return "Offer published successfully.";
        }
        else {
            return "Error publishing offer: " + response->body;
        }
    }

    void WebRTCPeerConnection::AcceptRemoteSDP(std::string sdp) {
        rtc::Description remoteSDP(sdp);
        _peerConnection->setRemoteDescription(sdp);
    }

    std::variant<std::monostate, bool, std::string> WebRTCPeerConnection::RetrieveOffer(const std::string& sessionID, const std::string& password) const
    {
        httplib::Client httpClient(SignallingServiceURL);

        httplib::Params httpParams = {
            {"sessionID", sessionID},
            {"password",password}
        };
        httplib::Headers httpHeaders{};

        auto response = httpClient.Get("/getOffer", httpParams, httpHeaders);

        if (response->status == 200) {
            return response->body;
        }
        else if (response->status == 403) {
            return false;
        }

        return std::monostate();
    }

    std::optional<std::string> WebRTCPeerConnection::RetrieveAnswer(const std::string& sessionID) const
    {
        httplib::Client httpClient(SignallingServiceURL);

        httplib::Params httpParams = {
            {"sessionID", sessionID}
        };
        httplib::Headers httpHeaders{};

        std::chrono::seconds pollingDuration(std::chrono::seconds::zero());
        std::chrono::seconds pollingInterval(1);

        do {
            auto response = httpClient.Get("/getAnswer", httpParams, httpHeaders);
            
            if (response->status == 200) {
                return response->body;
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
}
