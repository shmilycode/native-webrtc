#include "examples/peerconnection/peer_test/peerconnectionimpl.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture_factory.h"
#include "api/create_peerconnection_factory.h"
#include "pc/video_track_source.h"
#include "test/vcm_capturer.h"

class NoSenseSetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static NoSenseSetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<NoSenseSetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new rtc::RefCountedObject<CapturerTrackSource>(
            std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};


PeerConnectionImpl::PeerConnectionImpl(MainWindow* main_wnd)
  : offer_send_(false), main_wnd_(main_wnd){
    main_wnd->RegisterObserver(this);
    std::map<int, std::string> peers;
    main_wnd_->SwitchToPeerList(peers);
  }

void PeerConnectionImpl::SetRemotePeerConnection(rtc::scoped_refptr<PeerConnectionImpl> remote_peerconnection)
{
  remote_peer_connection_ = remote_peerconnection;
}

//for CreateSessionDescriptionObserver
void PeerConnectionImpl::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
  RTC_LOG(INFO) << __FUNCTION__;
  //offer_send_ flag would be set on 'client', whilch send the offer to the remote peer.
  //because of we are using the 'local' and 'remote' peer at the same process, so we need this 
  //flag to separate it.
  if(offer_send_) {
    SetLocalDescription(desc);
    remote_peer_connection_->SetRemoteDescription(desc);
    remote_peer_connection_->CreateAnswer();
  }
  else {
    SetLocalDescription(desc);
    remote_peer_connection_->SetRemoteDescription(desc);
  }
}

void PeerConnectionImpl::OnFailure(webrtc::RTCError error)
{
  RTC_LOG(INFO) << __FUNCTION__;
}

//for PeerConnectionObserver
void PeerConnectionImpl::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams)
{
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
                                 receiver->track().release());
}
void PeerConnectionImpl::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
  RTC_LOG(INFO) << __FUNCTION__;
}

void PeerConnectionImpl::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
  RTC_LOG(INFO) << __FUNCTION__;
}

//
// MainWndCallback implementation.
//
void PeerConnectionImpl::UIThreadCallback(int msg_id, void* data) {
  switch (msg_id) {
    case NEW_TRACK_ADDED: {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      RTC_LOG(INFO) << __FUNCTION__ << ": track kind is " << track->kind();
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        main_wnd_->StartRemoteRenderer(video_track);
      }
      track->Release();
      break;
    }
  }
}

//for myself
bool PeerConnectionImpl::InitPeerConnection()
{
  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
    nullptr /* network_thread */, nullptr /* worker_thread */,
    nullptr /* signaling_thread */, nullptr /* default_adm */,
    webrtc::CreateBuiltinAudioEncoderFactory(),
    webrtc::CreateBuiltinAudioDecoderFactory(),
    webrtc::CreateBuiltinVideoEncoderFactory(),
    webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
    nullptr /* audio_processing */);


  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = true;

  //create peerconnection
  peer_connection_ = peer_connection_factory_->CreatePeerConnection(
      config, nullptr, nullptr, this);
  if(!peer_connection_)
  {
    RTC_LOG(LS_ERROR) << "PeerConnection create failed";
    return false;
  }
  return true;
}

bool PeerConnectionImpl::AddTracks()
{
  RTC_LOG(INFO) << __FUNCTION__;
  //add audio track
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
  peer_connection_factory_->CreateAudioTrack(
      "audio_label", peer_connection_factory_->CreateAudioSource(
                       cricket::AudioOptions())));
  auto result_or_error = peer_connection_->AddTrack(audio_track, {"stream_id"});
  if (!result_or_error.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << result_or_error.error().message();
  }

  rtc::scoped_refptr<CapturerTrackSource> video_device =
      CapturerTrackSource::Create();

  if (video_device) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
        peer_connection_factory_->CreateVideoTrack("kVideoLabel", video_device));
    main_wnd_->StartLocalRenderer(video_track_);

    result_or_error = peer_connection_->AddTrack(video_track_, {"stream_id"});
    if (!result_or_error.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << result_or_error.error().message();
    }
  } else {
    RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
  }

  main_wnd_->SwitchToStreamingUI();
  return true;
}

void PeerConnectionImpl::CreateOffer()
{
  RTC_LOG(INFO) << __FUNCTION__;
  //create offer
  offer_send_ = true;
  peer_connection_->CreateOffer(this, 
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void PeerConnectionImpl::SetLocalDescription(webrtc::SessionDescriptionInterface* desc)
{
  RTC_LOG(INFO) << offer_send_ << " " << __FUNCTION__;
  peer_connection_->SetLocalDescription(
    NoSenseSetSessionDescriptionObserver::Create(), desc);
}

void PeerConnectionImpl::SetRemoteDescription(webrtc::SessionDescriptionInterface* desc)
{
  RTC_LOG(INFO) << offer_send_ << " " << __FUNCTION__;
  peer_connection_->SetRemoteDescription(
      NoSenseSetSessionDescriptionObserver::Create(),
      desc);
}

void PeerConnectionImpl::CreateAnswer(){
  RTC_LOG(INFO) << __FUNCTION__;
  peer_connection_->CreateAnswer(
      this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}
 
