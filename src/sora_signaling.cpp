#include "sora_signaling.h"
#include "sora_version.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <nlohmann/json.hpp>
#include <thread>
namespace {

std::string iceConnectionStateToString(
    webrtc::PeerConnectionInterface::IceConnectionState state) {
  switch (state) {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
      return "new";
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
      return "checking";
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
      return "connected";
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
      return "completed";
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
      return "failed";
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
      return "disconnected";
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
      return "closed";
    case webrtc::PeerConnectionInterface::kIceConnectionMax:
      return "max";
  }
  return "unknown";
}

}  // namespace

using json = nlohmann::json;

namespace sora {

webrtc::PeerConnectionInterface::IceConnectionState
SoraSignaling::getRTCConnectionState() const {
  return rtc_state_;
}

//Checks for iceconnection state and for the role and returns the appropriate rtconnection.
std::shared_ptr<RTCConnection> SoraSignaling::getRTCConnection()const{
    if (config_.multistream == true && config_.role == SoraSignalingConfig::Role::Sendrecv && !publishstreamId.empty()){
    if (connection_.at(publishstreamId)->getIceState() ==webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected ||
        connection_.at(publishstreamId)->getIceState() == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted) {
      return connection_.at(publishstreamId);
    }
  }
    if (config_.role == SoraSignalingConfig::Role::Recvonly && !playonlystreamId.empty()) {
    if (connection_.at(playonlystreamId)->getIceState() ==webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected ||
        connection_.at(playonlystreamId)->getIceState() ==webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted) {
      return connection_.at(playonlystreamId);
      }
    }
    if (config_.role == SoraSignalingConfig::Role::Sendonly&& !publishstreamId.empty()) {
      if (connection_.at(publishstreamId)->getIceState() == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected ||
          connection_.at(publishstreamId)->getIceState() ==webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted) {
        return connection_.at(publishstreamId);
      }
    }
    return nullptr;
}

std::shared_ptr<SoraSignaling> SoraSignaling::Create(
    boost::asio::io_context& ioc,
    RTCManager* manager,
    SoraSignalingConfig config,
    std::function<void(std::string)> on_notify) {
  auto p = std::shared_ptr<SoraSignaling>(
      new SoraSignaling(ioc, manager, config, std::move(on_notify)));
  if (!p->Init()) {
    return nullptr;
  }
  return p;
}

SoraSignaling::SoraSignaling(boost::asio::io_context& ioc,
                             RTCManager* manager,
                             SoraSignalingConfig config,
                             std::function<void(std::string)> on_notify)
    : ioc_(ioc),
      resolver_(ioc),
      manager_(manager),
      config_(config),
      on_notify_(std::move(on_notify)) {}

bool SoraSignaling::Init() {
  if (!URLParts::parse(config_.signaling_url, parts_)) {
    RTC_LOG(LS_ERROR) << "Invalid Signaling URL: " << config_.signaling_url;
    return false;
  }

  if (parts_.scheme != "wss") {
    RTC_LOG(LS_ERROR) << "Signaling URL Scheme is not secure web socket (wss): "
                      << config_.signaling_url;
    return false;
  }

  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);
  ssl_ctx.set_default_verify_paths();
  ssl_ctx.set_options(boost::asio::ssl::context::default_workarounds |
                      boost::asio::ssl::context::no_sslv2 |
                      boost::asio::ssl::context::no_sslv3 |
                      boost::asio::ssl::context::single_dh_use);

  wss_.reset(new ssl_websocket_t(ioc_, ssl_ctx));
  wss_->write_buffer_bytes(8192);

  // SNI
  if (!SSL_set_tlsext_host_name(wss_->next_layer().native_handle(),
                                parts_.host.c_str())) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                 boost::asio::error::get_ssl_category()};
    RTC_LOG(LS_ERROR) << "Failed SSL_set_tlsext_host_name: ec=" << ec;
    return false;
  }

  return true;
}
/*
Releases Sora.
*/
void SoraSignaling::release() {
  if (config_.role == SoraSignalingConfig::Role::Sendonly) {
    /*auto connection = std::move(connection_[publishstreamId]);
    connection = nullptr;*/
    connection_.clear();
    datachannels.clear();
  } else if (config_.multistream == true &&
             config_.role == SoraSignalingConfig::Role::Sendrecv) {
    connection_.clear();
    datachannels.clear();
  } else if (config_.role == SoraSignalingConfig::Role::Recvonly) {
    /*auto connection = std::move(connection_[playonlystreamId]);
    connection = nullptr;*/
    connection_.clear();
    datachannels.clear();
  }
}
/*
Connects to the websocket that you defined in Unity.
*/
bool SoraSignaling::connect() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (connected_) {
    return false;
  }

  std::string port = "5443";
  if (!parts_.port.empty()) {
    port = parts_.port;
  }

  RTC_LOG(LS_INFO) << "Start to resolve DNS: host=" << parts_.host
                   << " port=" << port;

  // DNS 
  resolver_.async_resolve(parts_.host, port,
                          boost::beast::bind_front_handler(
                              &SoraSignaling::onResolve, shared_from_this()));

  return true;
}

void SoraSignaling::onResolve(
    boost::system::error_code ec,
    boost::asio::ip::tcp::resolver::results_type results) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to resolve DNS: " << ec;
    return;
  }

  // DNS 
  boost::asio::async_connect(
      wss_->next_layer().next_layer(), results.begin(), results.end(),
      std::bind(&SoraSignaling::onSSLConnect, shared_from_this(),
                std::placeholders::_1));
}

void SoraSignaling::onSSLConnect(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to connect: " << ec;
    return;
  }

  // SSL
  wss_->next_layer().async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::beast::bind_front_handler(&SoraSignaling::onSSLHandshake,
                                       shared_from_this()));
}

void SoraSignaling::onSSLHandshake(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed SSL handshake: " << ec;
    return;
  }

  // Websocket 
  wss_->async_handshake(parts_.host, parts_.path_query_fragment,
                        boost::beast::bind_front_handler(
                            &SoraSignaling::onHandshake, shared_from_this()));
}

void SoraSignaling::onHandshake(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed Websocket handshake: " << ec;
    return;
  }

  connected_ = true;
  RTC_LOG(LS_INFO) << "Signaling Websocket is connected";

  doRead();
  doSendConnect();
}

#define SORA_CLIENT \
  "Sora Unity SDK " SORA_UNITY_SDK_VERSION " (" SORA_UNITY_SDK_COMMIT_SHORT ")"
#define LIBWEBRTC                                                      \
  "Shiguredo-build " WEBRTC_READABLE_VERSION " (" WEBRTC_BUILD_VERSION \
  " " WEBRTC_SRC_COMMIT_SHORT ")"

void SoraSignaling::doSendConnect() {
  std::string role =
      config_.role == SoraSignalingConfig::Role::Sendonly ? "publish" : "play";
  if (config_.multistream == true && config_.role == SoraSignalingConfig::Role::Sendrecv) {
    doSendJoinRoom(config_.channel_id);
  }
  else if (role == "publish") {
    doSendPublish(config_.channel_id);
  } else if (role == "play")
    doSendPlay(config_.channel_id);
}

void SoraSignaling::doSendJoinRoom(std::string str) {
  json json_message = {
      {"command", "joinRoom"},
      {"room", str}
  };
  sendText(json_message.dump());
}

void SoraSignaling::doSendPublish(std::string str) {
  json json_message = {
      {"command", "publish"},
      {"streamId", str},
      {"video", config_.audio_only==true?false:true},
  };
  sendText(json_message.dump());
}
void SoraSignaling::doSendPlay(std::string str) {
  json json_message = {
      {"command", "play"},
      {"streamId", str},
  };
  sendText(json_message.dump());
}
void SoraSignaling::doSendPong() {
  json json_message = {{"command", "ping"}};
  sendText(json_message.dump());
}
void SoraSignaling::doSendGetRoomInfo(std::string roomId, std::string str) {
  json json_message = {
      {"command", "getRoomInfo"},
      {"room", roomId},
      {"streamId", str},
  };
  sendText(json_message.dump());
}
                     
/*
Creates peer connection and sets streamid.
*/
void SoraSignaling::createPeerFromConfig(std::string streamId) {
  webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
  webrtc::PeerConnectionInterface::IceServers ice_servers;

  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = "stun:stun1.l.google.com:19302";
  ice_servers.push_back(ice_server);

  rtc_config.servers = ice_servers;
  playOnly = config_.role == SoraSignalingConfig::Role::Recvonly;
  if (!publishstreamId.empty())
    playOnly = true;
  connection_[streamId] = manager_->createConnection(rtc_config, this,streamId,config_.audio_only,playOnly);
  connection_[streamId]->setStreamId(streamId);
}

void SoraSignaling::close() {
  wss_->async_close(boost::beast::websocket::close_code::normal,
                    boost::beast::bind_front_handler(&SoraSignaling::onClose,
                                                     shared_from_this()));
}

void SoraSignaling::onClose(boost::system::error_code ec) {
  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to close: ec=" << ec;
    return;
  }
}

void SoraSignaling::doRead() {
  wss_->async_read(read_buffer_,
                   boost::beast::bind_front_handler(&SoraSignaling::onRead,
                                                    shared_from_this()));
}

void SoraSignaling::onRead(boost::system::error_code ec,
                           std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec;
    return;
  }

  const auto text = boost::beast::buffers_to_string(read_buffer_.data());
  read_buffer_.consume(read_buffer_.size());

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": text=" << text;
  /*
  Parsing the incoming websocket message and function according to message. If it is start, start publishing etc.
  */
  auto json_message = json::parse(text);
  const std::string command = json_message["command"];
  //Here is the where signaling handled
  //Start is for starting the publishing, creates peerconnection and sends offer. See observer.h and observer.cpp for callbacks in here for offer and answers. Also creates DataChannel.
  if (command == "start") {
    offer_sent_ = false;
    createPeerFromConfig(json_message["streamId"]);
    datachannels[json_message["streamId"]] =connection_[json_message["streamId"]]->createDataChannel(json_message["streamId"]);
    datachannels[json_message["streamId"]]->RegisterObserver(new DataChannelObserver(connection_[json_message["streamId"]]->getMessageSender(),json_message["streamId"]));
    connection_[json_message["streamId"]]->createOffer(json_message["streamId"],playOnly);
    offer_sent_ = true;
    publishstreamId = json_message["streamId"];
    connection_[publishstreamId]->setStreamId(publishstreamId);
  } else if (command == "takeConfiguration") {  // If playing, it will set remote offer and create answer. If publishing, it will set answer and that is all.
    if (json_message["type"] == "answer")
      connection_[json_message["streamId"]]->setAnswer(json_message["sdp"]);
    else if (json_message["type"] == "offer") {
      createPeerFromConfig(json_message["streamId"]);
      offer_sent_ = false;
      connection_[json_message["streamId"]]->setOffer(json_message["sdp"]);
      connection_[json_message["streamId"]]->createAnswer(json_message["streamId"]);
      playonlystreamId = json_message["streamId"];
    }
  } else if (command == "takeCandidate") { //Adds remote ice candidates to the peerconnection.
    ///*if (on_notify_) {
      //on_notify_(text);
    doSendPong();
    connection_[json_message["streamId"]]->addIceCandidate(
        json_message["id"], json_message["label"],
                                 json_message["candidate"]);
  }
  else if (command == "pong"){ //If pong message is arrived, try to read websocket again, if you are here without any incoming message, application may hang.
          if (rtc_state_ != webrtc::PeerConnectionInterface::IceConnectionState::
                          kIceConnectionConnected) {
      doRead();
      doSendPong();
      return;
    }

  } else if (command == "notification") {
    if (json_message["definition"] == "joinedTheRoom") {    // When joined to the room, it gets the streams from the room with 'streams' then plays them one by one. 
      doSendPublish(json_message["streamId"]);              //Also starting publishing the stream according to taken streamid from the server
      for (auto stream : json_message["streams"]) {
        doSendPlay(stream);
        playStreamIds.push_back(stream);
        }
    } else if (json_message["definition"] == "play_finished") {
      if (datachannels[json_message["streamId"]]) {
        datachannels[json_message["streamId"]]->UnregisterObserver();
      }
      connection_.erase(json_message["streamId"]);
      datachannels.erase(json_message["streamId"]);
      RTC_LOG(LS_ERROR) << "__FUNCTION__"
                        << "PLAY_FINISHED: "
                        << "stream "<<json_message["streamId"]<<"has been removed from the stream list";
    } else if (json_message["definition"] == "bitrateMeasurement") {
      doSendPong();
    }
  
  } else if (command == "roomInformation") {
    playStreamIds.clear();
    for (auto stream : json_message["streams"]) {
      if (connection_.find(stream) == connection_.end())
        doSendPlay(stream);
      playStreamIds.push_back(stream);
    }
  } else if(command=="error"){
    if (json_message["defition"] == "publishTimeoutError") {
      RTC_LOG(LS_ERROR) << "__FUNCTION__"
                        << "PUBLISH_TIMEOUT_ERROR: "
                        << "Publish stream is resetted";
      if (datachannels[publishstreamId]) {
        datachannels[publishstreamId]->UnregisterObserver();
        datachannels.erase(publishstreamId);
      }
      connection_.erase(publishstreamId);
    } else if (json_message["defition"] == "no_stream_exist") {
      RTC_LOG(LS_INFO) << "__FUNCTION__"
                        << "no_stream_exist: "
                        << "No stream has found with according stream id";

    }
  }
  doRead();
}
/*
Sends websocket message.
*/
void SoraSignaling::sendText(std::string text) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  boost::asio::post(boost::beast::bind_front_handler(
      &SoraSignaling::doSendText, shared_from_this(), std::move(text)));
}

void SoraSignaling::doSendText(std::string text) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": " << text;

  bool empty = write_buffer_.empty();
  boost::beast::flat_buffer buffer;

  const auto n = boost::asio::buffer_copy(buffer.prepare(text.size()),
                                          boost::asio::buffer(text));
  buffer.commit(n);

  write_buffer_.push_back(std::move(buffer));

  if (empty) {
    doWrite();
  }
}

void SoraSignaling::doWrite() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  auto& buffer = write_buffer_.front();

  wss_->text(true);
  wss_->async_write(buffer.data(),
                    boost::beast::bind_front_handler(&SoraSignaling::onWrite,
                                                     shared_from_this()));
}

void SoraSignaling::onWrite(boost::system::error_code ec,
                            std::size_t bytes_transferred) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to write: ec=" << ec;
    return;
  }

  write_buffer_.erase(write_buffer_.begin());

  if (!write_buffer_.empty()) {
    doWrite();
  }
}

/*
Functions at below are handled at observer.cpp.
*/
void SoraSignaling::onIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " state:" << new_state;
  boost::asio::post(boost::beast::bind_front_handler(
      &SoraSignaling::doIceConnectionStateChange, shared_from_this(),
      new_state));
}
void SoraSignaling::onIceCandidate(const std::string sdp_mid,
                                   const int sdp_mlineindex,
                                   const std::string sdp) {
  RTC_LOG(LS_INFO) << "__FUNCTION__"
                   << "Candidates are being added.";
}
void SoraSignaling::onCreateDescription(webrtc::SdpType type,
                                        const std::string sdp) {
 
RTC_LOG(LS_INFO) << __FUNCTION__ << " " << webrtc::SdpTypeToString(type);
}
void SoraSignaling::onSetDescription(webrtc::SdpType type) {
  RTC_LOG(LS_INFO) << __FUNCTION__
                   << " SdpType: " << webrtc::SdpTypeToString(type);
}

void SoraSignaling::doIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__
                   << ": oldState=" << iceConnectionStateToString(rtc_state_)
                   << ", newState=" << iceConnectionStateToString(new_state);

  switch (new_state) {
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionConnected:
      break;
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionFailed:
      break;
    default:
      break;
  }
  rtc_state_ = new_state;
}
/*
When remote peer opens a data channel, this callback will be executed. It creates a data channel with the incoming data channel and adds an observer to it to detect incoming messages.
*/
void SoraSignaling::onDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,std::string streamId) {
  datachannels[streamId] = data_channel;
  datachannels[streamId]->RegisterObserver(
      new DataChannelObserver(connection_[streamId]->getMessageSender(),streamId));
}

/*
Sends Data channel message. You can also send binary data and not only string but you may need to modify the way it binds with unity application.
*/
void sora::SoraSignaling::sendDataMessage(std::string streamId,std::string text) {
  if (datachannels[streamId]) {
    webrtc::DataBuffer buffer(text);
    datachannels[streamId]->Send(buffer);
  } else {
    RTC_LOG(LS_INFO) << __FUNCTION__;
    RTC_LOG(LS_ERROR)<< "Datachannel is not ready to send a message";
  }

  //RTC_LOG(LS_INFO)<<"sent data message is: " << buffer;
}

void sora::SoraSignaling::onMessage(const webrtc::DataBuffer& buffer,
    std::string streamId) {
    
  //rtc::CopyOnWriteBuffer buf = buffer.data;
  //char* c = buf.data<char>();
    on_notify_(std::string(buffer.data.data<char>()));
  RTC_LOG(LS_INFO) << std::string(buffer.data.data<char>());
}

}  // namespace sora
