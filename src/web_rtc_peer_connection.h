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

        /**
        * Publishes the local offer/answer session description to the signalling service.
        * The SDP is sent via a HTTP POST request.
        * This will make the offer or answer available to peers.
        *
        * @param type The offer/answer type of the SDP being published.
        * @param sessionID The identifier used for querying offer/answer SDPs for a session.
        * @return A message indicating that publishing was successful, or an error message.
        */
        std::string PublishSDP(const SDPType type, const std::string& sessionID, const std::string& password) const;

        /*
        * Receives session description information from a peer.
        * The remote sdp is set on the peer connection object.
        * If the peer instance is the initiater of the connection, the remote sdp is an answer.
        * If the peer instance did not initiate the connection, the remote sdp is an offer.
        *
        * @param remoteSDP The session description information recieved from a peer.
        */
        void AcceptRemoteSDP(std::string remoteSDP);

        /*
        * Queries the signalling service to retrieve an offer SDP for a given session identifier.
        * The response from the service depends on whether an offer exists, and whether the correct password is provided.
        *
        * If an offer exists, and the password is correct, the offer is returned.
        * If an offer exists, but the password is incorrect, the function returns false.
        * If an offer does not exist the function returns a std::monostate representing no value.
        *
        * @param sessionID The identifier used to query for an offer SDP.
        * @param password The password used to access the SDP.
        * @return The offer SDP string, false if the password is incorrect, or std::monostate if there is no offer.
        */
        std::variant<std::monostate, bool, std::string> RetrieveOffer(const std::string& sessionID, const std::string& password) const;

        /*
        * Queries the signalling service to retrieve an answer SDP for a given session identifier.
        * Peers expect to retrieve answers some amount of time following the publication of an offer.
        * Therefore this function will continue to periodically poll the service for an answer.
        * 
        * For the first 30 seconds of attempting connection, the service will be polled every second.
        * The polling interval is then increased to 5 seconds until 5 minutes of polling has elapsed.
        * Finally the polling interval is increased to 30 seconds until the maximum polling duration of 30 minutes has elapsed.
        * 
        * @param sessionID The session identifier to retrieve the answer for.
        * @return The answer SDP if it was successfully retrieved.
        */
        std::optional<std::string> RetrieveAnswer(const std::string& sessionID) const;

    private:
        rtc::Configuration _rtcConfig; // Configuration for the WebRTC connection. Contains the google STUN server address.
        std::shared_ptr<rtc::PeerConnection> _peerConnection; // The WebRTC peer connection.
        std::string _localSDP; // The local session description information to send to a peer.
    };
}
