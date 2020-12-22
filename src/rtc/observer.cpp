#include <iostream>
#include "rtc_base/logging.h"

#include "observer.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
namespace sora {
    
void PeerConnectionObserver::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  if (receiver_ == nullptr) {
    return;
  }
  auto track = transceiver->receiver()->track();
  if (track->kind() != webrtc::MediaStreamTrackInterface::kVideoKind) {
    return;
  }
  auto video_track = static_cast<webrtc::VideoTrackInterface*>(track.get());
  receiver_->AddTrack(video_track);
  video_tracks_.push_back(video_track);
}

void PeerConnectionObserver::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  if (receiver_ == nullptr) {
    return;
  }
  auto track = receiver->track();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    webrtc::VideoTrackInterface* video_track =
        static_cast<webrtc::VideoTrackInterface*>(track.get());
    video_tracks_.erase(
        std::remove_if(video_tracks_.begin(), video_tracks_.end(),
                       [video_track](const webrtc::VideoTrackInterface* track) {
                         return track == video_track;
                       }),
        video_tracks_.end());
    receiver_->RemoveTrack(video_track);
  }
}

void PeerConnectionObserver::ClearAllRegisteredTracks() {
  for (webrtc::VideoTrackInterface* video_track : video_tracks_) {
    receiver_->RemoveTrack(video_track);
  }
  video_tracks_.clear();
}

void PeerConnectionObserver::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  sender_->onIceConnectionStateChange(new_state);
}

void PeerConnectionObserver::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  if (candidate->ToString(&sdp)) {
    if (sender_ != nullptr) {
      sender_->onIceCandidate(candidate->sdp_mid(),
                              candidate->sdp_mline_index(), sdp);
      json json_message = {
      {"command", "takeCandidate"},
      {"streamId",
       streamId},
      {"label", candidate->sdp_mline_index()},
      {"id", candidate->sdp_mid()},
      {"candidate",sdp}}; 
  sender_->sendText(json_message.dump());
    }
  } else {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
  }
}

void CreateSessionDescriptionObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc) {
  std::string sdp;
  desc->ToString(&sdp);
  RTC_LOG(LS_INFO) << "Created session description : " << sdp;
  _connection->SetLocalDescription(
      SetSessionDescriptionObserver::Create(desc->GetType(), sender_), desc);
  if (sender_ != nullptr) {
    sender_->onCreateDescription(desc->GetType(), sdp);
    json json_message = {{"command", "takeConfiguration"},
                         {"streamId", streamId},
                         {"type", desc->GetType() == webrtc::SdpType::kAnswer?"answer":"offer"},
                         {"sdp", sdp}};
   
    sender_->sendText(json_message.dump());
  }
}

void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to create session description : "
                    << ToString(error.type()) << ": " << error.message();
}

void SetSessionDescriptionObserver::OnSuccess() {
  RTC_LOG(LS_INFO) << "Set local description success!";
  if (sender_ != nullptr) {
    sender_->onSetDescription(_type);
  }
}

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to set local description : "
                    << ToString(error.type()) << ": " << error.message();
}

void DataChannelObserver::OnMessage(const webrtc::DataBuffer& buffer) {
  sender_->onMessage(buffer,streamId);

}
/*void DataChannelObserver::OnBufferedAmountChange(uint64_t sent_data_size) {
  RTC_LOG(LS_INFO) << "Data Channel: "
                   << "sent data size is: "<< sent_data_size;

}*/

void PeerConnectionObserver::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
  sender_->onDataChannel(data_channel,streamId);
    }
}  // namespace sora
