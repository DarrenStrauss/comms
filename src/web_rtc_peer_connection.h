#pragma once

#include <string>
#include <optional>
#include <chrono>

#include "libdatachannel/rtc.hpp"

namespace Comms {

    enum class SDPType {
        Offer, // A peer's SDP is an offer if they are the first peer to initiate the call.
        Answer // A peer's SDP is an aswer if they have accepted an offer.
    };

    /*
    * Represents a WebRTC peer connection for an audio call.
    * Offer and Answer methods are provided for creating a peer to peer connection between two clients.
    * The connection will use Media Transport and is assumed to be for audio only using the OPUS codec.
    * The public google STUN server is used for IP address discovery.
    * Connections are identified by a user defined name and protected by a user defined password.
    */
    class WebRTCPeerConnection {
    public:
        /*
        * Constructor
        * 
        * @param name An identifier for this connection.
        * @param password A password used to grant access to this connection.
        */
        WebRTCPeerConnection(std::string name, std::string password);

        /*
        * Attemps to establish the WebRTC connection identified by the user defined name.
        * If no connection offer with this name has been made, this connection will make the offer and wait for a response.
        * If a connection offer has been made, this connection will attempt to accept the offer and establish the connection.
        * If an offer exists but the user defined password does not match the offer, this connection will be closed.
        * 
        * @return The state of the connection after attempting to establish the connection.
        */
        rtc::PeerConnection::State Connect();

    private:
        /*
        * Generates a local offer session description string.
        * This method should only be called on the peer instance initiating the connection.
        */
        void GenerateOfferSDP();

        /**
        * Publishes the local offer/answer session description to the signalling service.
        * The SDP is sent via a HTTP POST request.
        * This will make the offer or answer available to peers.
        *
        * @param type The offer/answer type of the SDP being published.
        */
        void PublishSDP(const SDPType type) const;

        /*
        * Queries the signalling service to retrieve an offer SDP for a given session identifier.
        * The response from the service depends on whether an offer exists, and whether the correct password is provided.
        *
        * If an offer exists, and the password is correct, the offer is returned.
        * If an offer exists, but the password is incorrect, the function returns false.
        * If an offer does not exist the function returns a std::monostate representing no value.
        *
        * @return The offer SDP string, false if the password is incorrect, or std::monostate if there is no offer.
        */
        std::variant<std::monostate, bool, std::string> RetrieveOffer() const;

        /*
        * Queries the signalling service to retrieve an answer SDP for a given session identifier.
        * Peers expect to retrieve answers some amount of time following the publication of an offer.
        * Therefore this function will continue to periodically poll the service for an answer.
        *
        * For the first 30 seconds of attempting connection, the service will be polled every second.
        * The polling interval is then increased to 5 seconds until 5 minutes of polling has elapsed.
        * Finally the polling interval is increased to 30 seconds until the maximum polling duration of 30 minutes has elapsed.
        *
        * @return The answer SDP if it was successfully retrieved.
        */
        std::optional<std::string> RetrieveAnswer() const;

        /*
        * Receives session description information from a peer.
        * The remote sdp is set on the peer connection object.
        * If the peer instance is the initiater of the connection, the remote sdp is an answer.
        * If the peer instance did not initiate the connection, the remote sdp is an offer.
        *
        * @param remoteSDP The session description information recieved from a peer.
        */
        void AcceptRemoteSDP(std::string remoteSDP) const;

        rtc::Configuration _rtcConfig; // Configuration for the WebRTC connection.
        std::unique_ptr<rtc::PeerConnection> _peerConnection; // The WebRTC peer connection.
        std::string _localSDP; // The local offer or answer session description information to send to a peer.
        const std::string _name; // The name used to identify a connection.
        const std::string _password; // The password used to grant access to the connection. 
    };
}
