# Sora Unity SDK


# Introduction

Ant Media Server is a streaming engine software that provides adaptive, ultra low latency streaming by using WebRTC technology with ~0.5 seconds latency. 
Ant Media Server is highly scalable both horizontally and vertically. It can run on-premise or on-cloud.
Ant Media Server - Scalable, Real-Time and Developer-friendly(API-First) - offers a robust and fully customizable environment to build your next live streaming product quickly.

## About Support

We check PRs or Issues only when written in ENGLISH.
In other languages, we won't be able to deal with them. Thank you for your understanding.



## Features

-Ultra Low Latency WebRTC Streaming
-Adaptive Bitrate Streaming
-Highly Scalable WebRTC Streaming on Clusters
-Free Live Streaming SDKs for iOS, Android, Unity and JavaScript
-Live Video Monitoring and Surveillance
-Open Source Live Streaming Software

## Licenses
Please visit out website for Licenses and choice as per your uses.(https://antmedia.io/)


We check PRs or Issues only when written in ENGLISH.
In other languages, we won't be able to deal with them. Thank you for your understanding.

## About AntMedia's open source software

Please read https://github.com/ant-media/sora-unity-sdk before using.

## How to Use
If you would like to use the Sora Unity SDK, please read [README.md] (doc / README.md).

## How to build

--Please read [BUILD_WINDOWS.md] (doc / BUILD_WINDOWS.md) for how to build on Windows.
--Please read [BUILD_MACOS.md] (doc / BUILD_MACOS.md) for how to build on macOS.

## Supported Unity versions

-All versions are supporting after Unity 2019.1

## Supported platforms
--Windows 10 1809 x86_64 or later
--macOS 10.15 x86_64 or later
--Android 7 or later
--iOS 10 or later


## Supported functions

--Support for Windows
--Support for macOS
--Compatible with Android
--iOS compatible
--Get Unity camera footage and send with Sora
--Get video from camera and send to Sora
--Get video from camera and output to Unity app
--Get audio from microphone and send to Sora
--Get audio from microphone and output to Unity app
--Receive audio from Sora in Unity app
--Receive video from Sora with the Unity app
--Play audio from Sora in Unity app
--Software encoding / decoding Support for VP8 / VP9
--Support for Opus
--Device specification function--Audio output from Unity instead of microphone
--Supports video acquisition from Unity camera
--Supports playback of audio received on the Unity side
--Output the audio received from Sora to the Unity app
--Output the video received from Sora to the Unity app
--Support for Sora multi-stream function
--Correspondence to Sora signaling notification
--Support for Sora metadata
--Supports audio codec / bit rate specification at the start of Sora signaling
--Supports video codec / bit rate specification at the start of Sora signaling
--Correspondence to signaling notification
--Apple Video Toolbox
--Support for H.264 hardware encoding
--Support for H.264 hardware decoding
--NVIDIA VIDEO CODEC SDK
--Windows version
--Support for H.264 hardware encoding
--Support for H.264 hardware decoding

## H.264 About the use of

H.264 encoding / decoding is not available in software with the Sora Unity SDK.
This is because if you distribute the software including the H.264 software encoder / decoder, you will be charged a license fee, so it is disabled.

Therefore, H.264 encoding / decoding is realized by using NVIDIA VIDEO CODEC SDK on Windows and VideoToolbox on macOS. Android also uses the H.264 hardware encoder.

Read H.264 [USE_H264.md] (doc / USE_H264.md) for more information


### platform
--Support for Ubuntu 18.04

### NVIDIA VIDEO CODEC SDK

--Support for VP8 hardware decoding
--Support for VP9 hardware decoding
--Support for Ubuntu 18.04

### Supports INTEL Media SDK

--Support for H.264 hardware encoding
--Support for VP8 hardware encoding
--Support for VP9 hardware encoding
--Support for H.264 hardware decoding
--Support for VP8 hardware decoding
--Support for VP9 hardware decoding