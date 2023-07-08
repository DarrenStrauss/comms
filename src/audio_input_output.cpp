#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_WASAPI
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

#include "audio_input_output.h"

namespace {
	const ma_format AudioFormat = ma_format_s16;
	const ma_uint32 AudioChannels = 1;
	const ma_uint32 AudioSampleRate = 48000;	
}

namespace Comms {
	AudioInputOutput::AudioInputOutput(std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> inputBuffer,
		std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> outputBuffer) :
		_inputBuffer(inputBuffer),
		_outputBuffer(outputBuffer) {
		_audioContext = std::unique_ptr<ma_context, std::function<void(ma_context*)>>(
			[]() {
				ma_context* context = new ma_context();
				ma_context_init(NULL, 0, NULL, context);
				return context;
			}(),
			[](ma_context* context) {
				ma_context_uninit(context);
				delete context;
			}
		);

		_inputDevice = std::unique_ptr<ma_device, std::function<void(ma_device*)>>(
			nullptr,
			[](ma_device* device) {
				ma_device_stop(device);
				ma_device_uninit(device);
				delete device;
			}
		);
		SetInputDevice(""); // The first available device

		_outputDevice = std::unique_ptr<ma_device, std::function<void(ma_device*)>>(
			nullptr,
			[](ma_device* device) {
				ma_device_stop(device);
				ma_device_uninit(device);
				delete device;
			}
		);
		SetOutputDevice("");
	}

	std::string AudioInputOutput::GetInputDeviceName() const {
		return _inputDeviceName;
	}

	std::string AudioInputOutput::GetOutputDeviceName() const {
		return _outputDeviceName;
	}

	void AudioInputOutput::SetInputDevice(std::string inputDeviceName) {
		for (const auto& device : GetDevices()) {
			if (device._isInput && (inputDeviceName.empty() || device._name == inputDeviceName)) {
				ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
				deviceConfig.capture.pDeviceID = &device._id;
				deviceConfig.capture.format = AudioFormat;
				deviceConfig.capture.channels = AudioChannels;
				deviceConfig.sampleRate = AudioSampleRate;
				deviceConfig.pUserData = static_cast<void*>(_inputBuffer.get());
				deviceConfig.dataCallback = ReadFromDevice;

				_inputDevice.reset(new ma_device());
				ma_device_init(_audioContext.get(), &deviceConfig, _inputDevice.get());
				_inputDeviceName = device._name;

				return;
			}
		}
	}

	void AudioInputOutput::SetOutputDevice(std::string outputDeviceName) {
		for (const auto& device : GetDevices()) {
			if (!device._isInput && (outputDeviceName.empty() || device._name == outputDeviceName)) {
				ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
				deviceConfig.playback.pDeviceID = &device._id;
				deviceConfig.playback.format = AudioFormat;
				deviceConfig.playback.channels = AudioChannels;
				deviceConfig.sampleRate = AudioSampleRate;
				deviceConfig.pUserData = static_cast<void*>(_outputBuffer.get());
				deviceConfig.dataCallback = WriteToDevice;

				_outputDevice.reset(new ma_device());
				ma_device_init(_audioContext.get(), &deviceConfig, _outputDevice.get());
				_outputDeviceName = device._name;

				return;
			}
		}
	}

	std::vector<std::string> AudioInputOutput::GetInputDeviceNames() const {
		std::vector<std::string> inputDeviceNames{};

		for (auto& device : GetDevices()) {
			if (device._isInput) {
				inputDeviceNames.push_back(device._name);
			}
		}

		return inputDeviceNames;
	}

	std::vector<std::string> AudioInputOutput::GetOutputDeviceNames() const {
		std::vector<std::string> outputDeviceNames{};

		for (auto& device : GetDevices()) {
			if (!device._isInput) {
				outputDeviceNames.push_back(device._name);
			}
		}

		return outputDeviceNames;
	}

	void AudioInputOutput::StartAudioStreams() {
		ma_device_start(_inputDevice.get());
		ma_device_start(_outputDevice.get());
	}

	void AudioInputOutput::StopAudioStreams()
	{
		ma_device_stop(_inputDevice.get());
		ma_device_stop(_outputDevice.get());
	}

	std::vector<AudioInputOutput::Device> AudioInputOutput::GetDevices() const {
		std::vector<AudioInputOutput::Device> devices{};

		ma_device_info* inputDevices = nullptr;
		ma_device_info* outputDevices = nullptr;
		ma_uint32 numInputDevices = 0;
		ma_uint32 numOutputDevices = 0;

		ma_context_get_devices(_audioContext.get(), &outputDevices, &numOutputDevices, &inputDevices, &numInputDevices);

		for (ma_uint32 i = 0; i < numInputDevices; i++) {
			AudioInputOutput::Device device;

			device._name = inputDevices[i].name;
			device._id = inputDevices[i].id;
			device._isInput = true;

			devices.push_back(device);
		}

		for (ma_uint32 i = 0; i < numOutputDevices; i++) {
			AudioInputOutput::Device device;

			device._name = outputDevices[i].name;
			device._id = outputDevices[i].id;
			device._isInput = false;

			devices.push_back(device);
		}

		return devices;
	}

	void AudioInputOutput::ReadFromDevice(ma_device* device, void* output, const void* input, ma_uint32 numFrames) {
		auto buffer = static_cast<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>*>(device->pUserData);
		const std::int16_t* samples = static_cast<const std::int16_t*>(input);

		for (ma_uint32 i = 0; i < numFrames; i++) {
			buffer->push(samples[i]);
		}
	}

	void AudioInputOutput::WriteToDevice(ma_device* device, void* output, const void* input, ma_uint32 numFrames) {
		auto buffer = static_cast<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>*>(device->pUserData);
		std::int16_t* samples = static_cast<std::int16_t*>(output);

		for (ma_uint32 i = 0; i < numFrames; i++) {
			std::int16_t sample = 0;

			if (buffer->pop(sample)) {
				samples[i] = sample;
			}
			else {
				samples[i] = 0; // Fill with silence if no available data
			}
		}
	}
}


