#ifndef SORA_RTC_MESSAGE_SENDER_H_
#define SORA_RTC_MESSAGE_SENDER_H_

#include <string>
#include "api/peer_connection_interface.h"

namespace sora {

class RTCMessageSender {
 public:
  virtual void onIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) = 0;
  virtual void onIceCandidate(const std::string sdp_mid,
                              const int sdp_mlineindex,
                              const std::string sdp) = 0;
  virtual void onCreateDescription(webrtc::SdpType type,
                                   const std::string sdp) = 0;
  virtual void onSetDescription(webrtc::SdpType type) = 0;
  virtual void sendText(std::string text) = 0;
  virtual void onDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,
      std::string streamId) = 0;
  virtual void onMessage(const webrtc::DataBuffer& buffer,
                         std::string streamId) = 0;
  virtual void sendDataMessage(std::string streamId, std::string text)=0;
};

}

#endif
