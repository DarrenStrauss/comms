#include "web_rtc_peer_connection.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include "json/json.hpp"

namespace {
    const char* StunServerURL = "stun:stun.l.google.com:19302";
    const char* SignallingServiceURL = "https://australia-southeast1-comms-link.cloudfunctions.net";
}

using json = nlohmann::json;

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

std::string WebRTCPeerConnection::PublishOfferSDP(const std::string& sessionID, const std::string& password = "") const
{
    json httpBody = {
        {"sessionID", sessionID},
        {"password", password},
        {"offer", _localSDP}
    };

    httplib::Client httpClient(SignallingServiceURL);
    auto response = httpClient.Post("/sessionOffer", httpBody.dump(), "application/json");

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