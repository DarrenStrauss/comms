#include "web_rtc_peer_connection.h"

namespace {
    const char* StunServerURL = "stun:stun.l.google.com:19302";
}

WebRTCPeerConnection::WebRTCPeerConnection() :
    _rtcConfig(),
    _localSDP("") {
    rtc::InitLogger(rtc::LogLevel::Debug);

    _rtcConfig.iceServers.emplace_back(StunServerURL);
    _peerConnection = std::make_shared<rtc::PeerConnection>(_rtcConfig);
}

void WebRTCPeerConnection::GenerateLocalSDP() {

    _peerConnection->onGatheringStateChange([&](rtc::PeerConnection::GatheringState state) {

        if (state == rtc::PeerConnection::GatheringState::Complete) {
            auto description = _peerConnection->localDescription();
            _localSDP = std::string(description.value());
        }
    });

    rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
    media.addOpusCodec(96);
    media.setBitrate(256);
    auto track = _peerConnection->addTrack(media);

    _peerConnection->setLocalDescription();
}

std::string WebRTCPeerConnection::GetLocalSDP() {
    return _localSDP;
}

void WebRTCPeerConnection::AcceptRemoteSDP(std::string sdp) {
    rtc::Description remoteSDP(sdp);
    _peerConnection->setRemoteDescription(sdp);
}