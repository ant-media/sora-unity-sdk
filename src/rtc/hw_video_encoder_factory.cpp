#include "hw_video_encoder_factory.h"

#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "rtc_base/logging.h"

#include "h264_format.h"
#if defined(SORA_UNITY_SDK_WINDOWS)
#include "hwenc_nvcodec/nvcodec_h264_encoder.h"
#endif

namespace sora {

std::vector<webrtc::SdpVideoFormat> HWVideoEncoderFactory::GetSupportedFormats()
    const {
  std::vector<webrtc::SdpVideoFormat> supported_codecs;
  supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
    supported_codecs.push_back(format);

#if defined(SORA_UNITY_SDK_WINDOWS)
  if (NvCodecH264Encoder::IsSupported()) {
    std::vector<webrtc::SdpVideoFormat> h264_codecs = {
        CreateH264Format(webrtc::H264::kProfileBaseline,
                         webrtc::H264::kLevel3_1, "1"),
        CreateH264Format(webrtc::H264::kProfileBaseline,
                         webrtc::H264::kLevel3_1, "0"),
        CreateH264Format(webrtc::H264::kProfileConstrainedBaseline,
                         webrtc::H264::kLevel3_1, "1"),
        CreateH264Format(webrtc::H264::kProfileConstrainedBaseline,
                         webrtc::H264::kLevel3_1, "0")};

    for (const webrtc::SdpVideoFormat& format : h264_codecs)
      supported_codecs.push_back(format);
  }
#endif

  return supported_codecs;
}

std::unique_ptr<webrtc::VideoEncoder> HWVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
    return webrtc::VP8Encoder::Create();

  if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
    return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));

#if defined(SORA_UNITY_SDK_WINDOWS)
  if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName)) {
    return std::unique_ptr<webrtc::VideoEncoder>(
        absl::make_unique<NvCodecH264Encoder>(cricket::VideoCodec(format)));
  }
#endif

  RTC_LOG(LS_ERROR) << "Trying to created encoder of unsupported format "
                    << format.name;
  return nullptr;
}

}  // namespace sora
