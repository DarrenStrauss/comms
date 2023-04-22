#pragma once

#include <string>

#include "libdatachannel/rtc.hpp"

/*
* Represents a WebRTC peer connection.
* Methods are provided for negotiating and creating a peer to peer connection between two clients.
* The connection will use Media Transport and is assumed to be for audio only.
*/
class WebRTCPeerConnection {
public:
    WebRTCPeerConnection();

    /*
    * Generates the audio session description information to offer to a peer.
    * The public google STUN server is used for IP address discovery.
    * The description will specify an audio connection using the OPUS codec.
    */
    void GenerateOfferSDP();

    /*
    * Returns the session description information as a string.
    * If the information is not yet available, an empty string will be returned.
    */
    std::string GetOfferSDP();
private:
    rtc::Configuration _rtcConfig; // Configuration for the connection. Contains the google STUN server address.
    std::string _offerSDP; // The session description information to offer to a peer.
};
