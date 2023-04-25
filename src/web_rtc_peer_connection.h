#pragma once

#include <string>

#include "libdatachannel/rtc.hpp"

/*
* Represents a WebRTC peer connection for an audio call.
* Methods are provided for creating a peer to peer connection between two clients.
* The connection will use Media Transport and is assumed to be for audio only.
*/
class WebRTCPeerConnection {
public:
    WebRTCPeerConnection();

    /*
    * Generates the audio session description information to send to a peer.
    * The public google STUN server is used for IP address discovery.
    * The description will specify an audio connection using the OPUS codec.
    */
    void GenerateLocalSDP();

    /*
    * Gets the generated session description information as a string.
    * If the information is not yet available, an empty string will be returned.
    * 
    * @return The sdp if it has been generated, otherwise an empty string.
    */
    std::string GetLocalSDP();

    /*
    * Saves the session description information recieved from a peer.
    */
    void AcceptRemoteSDP(std::string remoteSDP);

private:
    rtc::Configuration _rtcConfig; // Configuration for the connection. Contains the google STUN server address.
    std::shared_ptr<rtc::PeerConnection> _peerConnection; // The peer connection.
    std::string _localSDP; // The session description information to send to a peer.
};
