#ifndef PEERTEST_PEERCONNECTIONIMPL_H_
#define PEERTEST_PEERCONNECTIONIMPL_H_
#include <gtk/gtk.h>
#include "examples/peerconnection/peer_test/linux/main_wnd.h"
#include "api/peerconnectioninterface.h"

class PeerConnectionImpl : public webrtc::CreateSessionDescriptionObserver,
                           public webrtc::PeerConnectionObserver,
                           public MainWndCallback
{
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_TRACK_ADDED,
    TRACK_REMOVED,
  };

  PeerConnectionImpl(MainWindow* main_wnd);
  void SetRemotePeerConnection(rtc::scoped_refptr<PeerConnectionImpl> remote_peerconnection);
  //for CreateSessionDescriptionObserver
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;
  //for PeerConnectionObserver
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override{};
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
 void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
 void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override{};
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override{};
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // MainWndCallback implementation.
  //
  void UIThreadCallback(int msg_id, void* data) override;
  void StartLogin(const std::string& server, int port) override {}
  void DisconnectFromServer() override {}
  void ConnectToPeer(int peer_id) override {}
  void DisconnectFromCurrentPeer() override {}
  void Close() override{}

  //for myself
  bool InitPeerConnection();
  std::unique_ptr<cricket::VideoCapturer> OpenVideoCaptureDevice();
  bool AddTracks();
  void CreateOffer();
  void SetLocalDescription(webrtc::SessionDescriptionInterface* desc);
  void SetRemoteDescription(webrtc::SessionDescriptionInterface* desc);
  void CreateAnswer();

 protected:
  ~PeerConnectionImpl(){}
 private:
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<PeerConnectionImpl> remote_peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;

  //use this flag to separate from client/server
  //the client would send the offer, so this flag would be set to 'true'
  //the sever would keep the 'false' value
  bool offer_send_;
  MainWindow* main_wnd_;
};
#endif