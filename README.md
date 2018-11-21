# Complete vidoe capture with webrtc natvie api, and a simple Gtk UI.

## Must be complie with Webrtc

compile with gn command: 
```
  rtc_executable("peer_test") {
    testonly = true
    sources = [
    "peerconnection/peer_test/peerconnectionimpl.cc",
    "peerconnection/peer_test/peerconnectionimpl.h",
    "peerconnection/peer_test/main.cc",
    ]
    if (!build_with_chromium && is_clang) {
      # Suppress warnings from the Chromium Clang plugin (bugs.webrtc.org/163).
      suppressed_configs += [ "//build/config/clang:find_bad_constructs" ]
    }
    deps = [
      "../api:libjingle_peerconnection_api",
      "../api/video:video_frame_i420",
      "../rtc_base:checks",
      "../rtc_base:stringutils",
      "../rtc_base/third_party/sigslot",
      "../system_wrappers:field_trial",
      "../test:field_trial",
    ]
    sources += [
      "peerconnection/peer_test/linux/main_wnd.cc",
      "peerconnection/peer_test/linux/main_wnd.h",
    ]
    cflags = [ "-Wno-deprecated-declarations" ]
    libs = [
      "X11",
      "Xcomposite",
      "Xext",
      "Xrender",
    ]
    deps += [ "//build/config/linux/gtk" ]
    deps += [
      "../api:libjingle_peerconnection_api",
      "../api/audio_codecs:builtin_audio_decoder_factory",
      "../api/audio_codecs:builtin_audio_encoder_factory",
      "../api/video:video_frame",
      "../api/video_codecs:builtin_video_decoder_factory",
      "../api/video_codecs:builtin_video_encoder_factory",
      "../media:rtc_audio_video",
      "../modules/audio_device:audio_device",
      "../modules/audio_processing:api",
      "../modules/audio_processing:audio_processing",
      "../modules/video_capture:video_capture_module",
      "../pc:libjingle_peerconnection",
      "../rtc_base:rtc_base",
      "../rtc_base:rtc_base_approved",
      "../rtc_base:rtc_json",
      "//third_party/libyuv",
    ]
  }
```

The original possition I play this code is in `src/examples/peerconnection/peer_test`, so you should change the relative path in the gn above.

## Example was change from [google examples](https://webrtc.googlesource.com/src/+/master/examples/peerconnection/)

Implement withouth signal channel to exchange device description, and other complex meso it's easier to learn how the peerconnection work, and this is the essential reason why I write it.