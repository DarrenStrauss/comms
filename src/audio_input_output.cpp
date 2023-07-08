#include "audio_input_output.h"

#include <array>
#include <cstdint>

namespace Comms {
	AudioInputOutput::AudioInputOutput() {

	}

	char* AudioInputOutput::GetInputDeviceName() const {
		return nullptr;
	}

	char* AudioInputOutput::GetOutputDeviceName() const {
		return nullptr;
	}

	void AudioInputOutput::SetInputDevice(const char* inputDeviceName) {
		
	}

	void AudioInputOutput::SetOutputDevice(const char* outputDeviceName) {
		
	}

	std::vector<std::string> AudioInputOutput::GetAvailableInputDeviceNames() const {
		return std::vector<std::string>(); // If no selected input device, there are no available input devices on the system.
	}

	std::vector<std::string> AudioInputOutput::GetAvailableOutputDeviceNames() const {
		return std::vector<std::string>(); // If no selected output device, there are no available output devices on the system.
	}

	void AudioInputOutput::StartAudioStreams(std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> inputData,
		std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> outputData) {

		
	}

	void AudioInputOutput::StopAudioStreams()
	{
		
	}

	int AudioInputOutput::GetDeviceCount(DeviceType deviceType) const {
		return 0;
	}

	void AudioInputOutput::GetDevice(DeviceType deviceType, int deviceIndex) const {
		return;
	}

	std::vector<std::string> AudioInputOutput::GetAvailableDeviceNames(DeviceType deviceType, std::string currentDeviceName, int currentDeviceIndex) const {
		std::vector<std::string> deviceNames{};

		return deviceNames;
	}

	void AudioInputOutput::SetDevice(DeviceType deviceType, const char* newDeviceName) {
		
	}
}


