#pragma once

#include <string>

#include "libdatachannel/rtc.hpp"

/*
* Represents a WebRTC peer connection for an audio call.
* Offer and Answer methods are provided for creating a peer to peer connection between two clients.
* The connection will use Media Transport and is assumed to be for audio only using the OPUS codec.
* The public google STUN server is used for IP address discovery.
*/
class WebRTCPeerConnection {
public:
    WebRTCPeerConnection();

    /*
    * Gets the local session description information as a string.
    * If the information is not yet available, an empty string will be returned.
    * Local SDP information will represent a connection "offer" if GenerateOfferSDP is called before AcceptRemoteSDP.
    * Otherwise if AcceptRemoteSDP is called before GenerateOfferSDP, the local SDP will represent an "answer".
    * 
    * @return The offer or answer sdp if it has been generated, otherwise an empty string.
    */
    std::string GetLocalSDP();

    /*
    * Generates a local offer session description string.
    * This method should only be called on the peer instance initiating the connection.
    */
    void GenerateOfferSDP();

    /*
    * Receives session description information from a peer.
    * The remote sdp is set on the peer connection object.
    * If the peer instance is the initiater of the connection, the remote sdp is an answer.
    * If the peer instance did not initiate the connection, the remote sdp is an offer.
    * 
    * @param remoteSDP The session description information recieved from a peer.
    */
    void AcceptRemoteSDP(std::string remoteSDP);

private:
    rtc::Configuration _rtcConfig; // Configuration for the connection. Contains the google STUN server address.
    std::shared_ptr<rtc::PeerConnection> _peerConnection; // The peer connection.
    std::string _localSDP; // The session description information to send to a peer.
};
