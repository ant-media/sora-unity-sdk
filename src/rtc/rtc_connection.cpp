#include "rtc_base/logging.h"

#include "rtc_connection.h"

namespace sora {

// stats のコールバックを受け取るためのクラス
class RTCStatsCallback : public webrtc::RTCStatsCollectorCallback {
 public:
  typedef std::function<void(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)>
      ResultCallback;

  static RTCStatsCallback* Create(ResultCallback result_callback) {
    return new rtc::RefCountedObject<RTCStatsCallback>(
        std::move(result_callback));
  }

  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override {
    std::move(result_callback_)(report);
  }

 protected:
  RTCStatsCallback(ResultCallback result_callback)
      : result_callback_(std::move(result_callback)) {}
  ~RTCStatsCallback() override = default;

 private:
  ResultCallback result_callback_;
};

RTCConnection::~RTCConnection() {
  connection_->Close();
}

void RTCConnection::createOffer(std::string streamId, bool playOnly) {
  using RTCOfferAnswerOptions =
      webrtc::PeerConnectionInterface::RTCOfferAnswerOptions;
  RTCOfferAnswerOptions options = RTCOfferAnswerOptions();
  options.offer_to_receive_video = playOnly==true? RTCOfferAnswerOptions::kOfferToReceiveMediaTrue
                       : 0;
  options.offer_to_receive_audio =playOnly == true ?
      RTCOfferAnswerOptions::kOfferToReceiveMediaTrue:0;
  connection_->CreateOffer(
      CreateSessionDescriptionObserver::Create(sender_, connection_,streamId), options);
}

void RTCConnection::setOffer(const std::string sdp) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
      webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp, &error);
  if (!session_description) {
    RTC_LOG(LS_ERROR) << __FUNCTION__
                      << "Failed to create session description: "
                      << error.description.c_str()
                      << "\nline: " << error.line.c_str();
    return;
  }
  connection_->SetRemoteDescription(
      SetSessionDescriptionObserver::Create(session_description->GetType(),
                                            sender_),
      session_description.release());
}

void RTCConnection::createAnswer(std::string streamId) {
  connection_->CreateAnswer(
      CreateSessionDescriptionObserver::Create(sender_, connection_,streamId),
      webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void RTCConnection::setAnswer(const std::string sdp) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
      webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp, &error);
  if (!session_description) {
    RTC_LOG(LS_ERROR) << __FUNCTION__
                      << "Failed to create session description: "
                      << error.description.c_str()
                      << "\nline: " << error.line.c_str();
    return;
  }
  connection_->SetRemoteDescription(
      SetSessionDescriptionObserver::Create(session_description->GetType(),
                                            sender_),
      session_description.release());
}

void RTCConnection::addIceCandidate(const std::string sdp_mid,
                                    const int sdp_mlineindex,
                                    const std::string sdp) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::IceCandidateInterface> candidate(
      webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
  if (!candidate.get()) {
    RTC_LOG(LS_ERROR) << "Can't parse received candidate message: "
                      << error.description.c_str()
                      << "\nline: " << error.line.c_str();
    return;
  }
  if (!connection_->AddIceCandidate(candidate.get())) {
    RTC_LOG(LS_WARNING) << __FUNCTION__
                        << "Failed to apply the received candidate : " << sdp;
    return;
  }
}

bool RTCConnection::setAudioEnabled(bool enabled) {
  return setMediaEnabled(getLocalAudioTrack(), enabled);
}

bool RTCConnection::setVideoEnabled(bool enabled) {
  return setMediaEnabled(getLocalVideoTrack(), enabled);
}

bool RTCConnection::isAudioEnabled() {
  return isMediaEnabled(getLocalAudioTrack());
}

bool RTCConnection::isVideoEnabled() {
  return isMediaEnabled(getLocalVideoTrack());
}

void RTCConnection::setStreamId(std::string streamId) {
  this->streamId = streamId;
}

std::string RTCConnection::getStreamId() {
  return streamId;
}

rtc::scoped_refptr<webrtc::MediaStreamInterface>
RTCConnection::getLocalStream() {
  return connection_->local_streams()->at(0);
}

rtc::scoped_refptr<webrtc::AudioTrackInterface>
RTCConnection::getLocalAudioTrack() {
  rtc::scoped_refptr<webrtc::MediaStreamInterface> local_stream =
      getLocalStream();
  if (!local_stream) {
    return nullptr;
  }

  if (local_stream->GetAudioTracks().size() > 0) {
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
        local_stream->GetAudioTracks()[0]);
    if (audio_track) {
      return audio_track;
    }
  }
  return nullptr;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface>
RTCConnection::getLocalVideoTrack() {
  rtc::scoped_refptr<webrtc::MediaStreamInterface> local_stream =
      getLocalStream();
  if (!local_stream) {
    return nullptr;
  }

  if (local_stream->GetVideoTracks().size() > 0) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
        local_stream->GetVideoTracks()[0]);
    if (video_track) {
      return video_track;
    }
  }
  return nullptr;
}

bool RTCConnection::setMediaEnabled(
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
    bool enabled) {
  if (track) {
    return track->set_enabled(enabled);
  }
  return false;
}

bool RTCConnection::isMediaEnabled(
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
  if (track) {
    return track->enabled();
  }
  return false;
}

void RTCConnection::getStats(
    std::function<void(const rtc::scoped_refptr<const webrtc::RTCStatsReport>&)>
        callback) {
  connection_->GetStats(RTCStatsCallback::Create(std::move(callback)));
}
webrtc::PeerConnectionInterface::IceConnectionState sora::RTCConnection::getIceState() {
  return connection_->ice_connection_state();
}
rtc::scoped_refptr<webrtc::DataChannelInterface> RTCConnection::createDataChannel(std::string streamId) {
  webrtc::DataChannelInit* options = {};
  return connection_->CreateDataChannel(streamId, options);
}
RTCMessageSender* sora::RTCConnection::getMessageSender() {
  return sender_;
}
}  // namespace sora
