#include "web_rtc_peer_connection.h"

namespace {
    const char* StunServerURL = "stun:stun.l.google.com:19302";
}

WebRTCPeerConnection::WebRTCPeerConnection() :
    _rtcConfig(),
    _offerSDP("") {
    rtc::InitLogger(rtc::LogLevel::Debug);
    _rtcConfig.iceServers.emplace_back(StunServerURL);
}

void WebRTCPeerConnection::GenerateOfferSDP() {
    auto offerConnection = std::make_shared<rtc::PeerConnection>(_rtcConfig);

    offerConnection->onGatheringStateChange([this, offerConnection](rtc::PeerConnection::GatheringState state) {

        if (state == rtc::PeerConnection::GatheringState::Complete) {
            auto description = offerConnection->localDescription();
            _offerSDP = std::string(description.value());
        }
    });

    rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
    media.addOpusCodec(96);
    media.setBitrate(256);
    auto track = offerConnection->addTrack(media);

    offerConnection->setLocalDescription();
}

std::string WebRTCPeerConnection::GetOfferSDP() {
    return _offerSDP;
}