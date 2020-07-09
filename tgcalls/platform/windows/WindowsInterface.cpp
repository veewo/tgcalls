#include "WindowsInterface.h"

#include "api/peer_connection_interface.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/peer_connection_interface.h"
#include "api/video_track_source_proxy.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "pc/video_track_source.h"
#include "sdk/media_constraints.h"
#include "test/platform_video_capturer.h"

#include "VideoCapturerInterfaceImpl.h"

namespace tgcalls {
namespace {

class CapturerTrackSource : public webrtc::VideoTrackSource {
public:
	static rtc::scoped_refptr<CapturerTrackSource> Create();

protected:
	explicit CapturerTrackSource(
		std::unique_ptr<webrtc::test::TestVideoCapturer> capturer)
		: VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
		return capturer_.get();
	}
	std::unique_ptr<webrtc::test::TestVideoCapturer> capturer_;

};

rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create() {
	const size_t kWidth = 640;
	const size_t kHeight = 480;
	const size_t kFps = 30;
#ifndef TARGET_OS_OSX
	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		webrtc::VideoCaptureFactory::CreateDeviceInfo());
	if (!info) {
		return nullptr;
	}
	int num_devices = info->NumberOfDevices();
#else // TARGET_OS_OSX
	int num_devices = 1;
#endif // TARGET_OS_OSX
	for (int i = 0; i < num_devices; ++i) {
		if (auto capturer = webrtc::test::CreateVideoCapturer(kWidth, kHeight, kFps, i)) {
			return new rtc::RefCountedObject<CapturerTrackSource>(
				std::move(capturer));
		}
	}
	return nullptr;
}

} // namespace

std::unique_ptr<webrtc::VideoEncoderFactory> WindowsInterface::makeVideoEncoderFactory() {
	return webrtc::CreateBuiltinVideoEncoderFactory();
}

std::unique_ptr<webrtc::VideoDecoderFactory> WindowsInterface::makeVideoDecoderFactory() {
	return webrtc::CreateBuiltinVideoDecoderFactory();
}

rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> WindowsInterface::makeVideoSource(rtc::Thread *signalingThread, rtc::Thread *workerThread) {
	const auto videoTrackSource = CapturerTrackSource::Create();
	return webrtc::VideoTrackSourceProxy::Create(signalingThread, workerThread, videoTrackSource);
}

bool WindowsInterface::supportsH265Encoding() {
	return false;
}

std::unique_ptr<VideoCapturerInterface> WindowsInterface::makeVideoCapturer(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source, bool useFrontCamera, std::function<void(bool)> isActiveUpdated) {
	return std::make_unique<VideoCapturerInterfaceImpl>(source, useFrontCamera, isActiveUpdated);
}

std::unique_ptr<PlatformInterface> CreatePlatformInterface() {
	return std::make_unique<WindowsInterface>();
}

} // namespace tgcalls