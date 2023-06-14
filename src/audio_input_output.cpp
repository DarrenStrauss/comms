#include "audio_input_output.h"

#include <array>
#include <cstdint>

namespace {
	constexpr std::array<SoundIoFormat, 3> PrioritizedFormats {
		SoundIoFormatS16NE,
		SoundIoFormatS16FE,
		SoundIoFormatInvalid
	};

	constexpr std::array<int, 5> PrioritizedSampleRates {
		48000,
		44100,
		96000,
		24000,
		0
	};
}

namespace Comms {
	AudioInputOutput::AudioInputOutput() :
		_soundio(soundio_create(), &soundio_destroy) {

		soundio_connect_backend(_soundio.get(), SoundIoBackendWasapi);
		soundio_flush_events(_soundio.get());

		_inputDeviceIndex = soundio_default_input_device_index(_soundio.get());

		_inputDevice = std::unique_ptr<SoundIoDevice, std::function<void(SoundIoDevice*)>>(
			soundio_get_input_device(_soundio.get(), _inputDeviceIndex),
			&soundio_device_unref
		);

		SetDeviceFormat(*_inputDevice, DeviceType::Input);
		SetDeviceSampleRate(*_inputDevice, DeviceType::Input);

		_outputDeviceIndex = soundio_default_output_device_index(_soundio.get());

		_outputDevice = std::unique_ptr<SoundIoDevice, std::function<void(SoundIoDevice*)>>(
			soundio_get_output_device(_soundio.get(), _outputDeviceIndex),
			&soundio_device_unref
		);

		SetDeviceFormat(*_outputDevice, DeviceType::Output);
		SetDeviceSampleRate(*_outputDevice, DeviceType::Output);
	}

	char* AudioInputOutput::GetInputDeviceName() const {
		return _inputDevice ? _inputDevice->name : nullptr;
	}

	char* AudioInputOutput::GetOutputDeviceName() const {
		return _outputDevice ? _outputDevice->name : nullptr;
	}

	void AudioInputOutput::SetInputDevice(const char* inputDeviceName) {
		SetDevice(DeviceType::Input, inputDeviceName);
	}

	void AudioInputOutput::SetOutputDevice(const char* outputDeviceName) {
		SetDevice(DeviceType::Output, outputDeviceName);
	}

	std::vector<std::string> AudioInputOutput::GetAvailableInputDeviceNames() const {
		if (_inputDevice) {
			return GetAvailableDeviceNames(DeviceType::Input, _inputDevice->name, _inputDeviceIndex);
		}

		return std::vector<std::string>(); // If no selected input device, there are no available input devices on the system.
	}

	std::vector<std::string> AudioInputOutput::GetAvailableOutputDeviceNames() const {
		if (_outputDevice) {
			return GetAvailableDeviceNames(DeviceType::Output, _outputDevice->name, _outputDeviceIndex);
		}

		return std::vector<std::string>(); // If no selected output device, there are no available output devices on the system.
	}

	void AudioInputOutput::StartAudioStreams(std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> inputData,
		std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> outputData) {

		_inputStream = std::unique_ptr<SoundIoInStream, std::function<void(SoundIoInStream*)>>(
			soundio_instream_create(_inputDevice.get()),
			&soundio_instream_destroy
		);
		_inputStream->format = _inputFormat;
		_inputStream->sample_rate = _inputSampleRate;
		_inputStream->userdata = static_cast<void*>(inputData.get());
		_inputStream->software_latency = 0.0;

		_inputStream->read_callback = [](SoundIoInStream* inputStream, int minSampleCount, int maxSampleCount) {
			int numSamplesRemaining = maxSampleCount;
			SoundIoChannelArea* dataArea;
			auto inputData = static_cast<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>*>(inputStream->userdata);

			while (numSamplesRemaining > 0) {
				int numSamplesToRead = numSamplesRemaining;
				soundio_instream_begin_read(inputStream, &dataArea, &numSamplesToRead);

				if (numSamplesToRead == 0) {
					break;
				}

				for (int i = 0; i < numSamplesToRead; i++) {
					std::int16_t sample;
					memcpy(&sample, dataArea[0].ptr, inputStream->bytes_per_sample);
					inputData->push(sample);
					dataArea[0].ptr += dataArea[0].step;
				}

				soundio_instream_end_read(inputStream);
				numSamplesRemaining -= numSamplesToRead;
			}
		};

		_outputStream = std::unique_ptr<SoundIoOutStream, std::function<void(SoundIoOutStream*)>>(
			soundio_outstream_create(_outputDevice.get()),
			&soundio_outstream_destroy
		);
		_outputStream->format = _outputFormat;
		_outputStream->sample_rate = _outputSampleRate;
		_outputStream->userdata = static_cast<void*>(outputData.get());
		_outputStream->software_latency = 0.0;

		_outputStream->write_callback = [](SoundIoOutStream* outputStream, int minFrameCount, int maxFrameCount) {
			int numFramesRemaining = maxFrameCount;
			SoundIoChannelArea* dataArea;
			auto outputData = static_cast<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>*>(outputStream->userdata);

			while (numFramesRemaining > 0) {
				int numFramesToWrite = numFramesRemaining;
				soundio_outstream_begin_write(outputStream, &dataArea, &numFramesToWrite);

				if (numFramesToWrite == 0) {
					break;
				}

				for (int i = 0; i < numFramesToWrite; i++) {
					std::int16_t sample = outputData->front();
					outputData->pop();

					for (int j = 0; j < outputStream->layout.channel_count; j++) {
						memcpy(dataArea[j].ptr, &sample, outputStream->bytes_per_sample);
						dataArea[j].ptr += dataArea[j].step;
					}
				}

				soundio_outstream_end_write(outputStream);
				numFramesRemaining -= numFramesToWrite;
			}
		};

		soundio_instream_open(_inputStream.get());
		soundio_outstream_open(_outputStream.get());

		soundio_instream_start(_inputStream.get());
		soundio_outstream_start(_outputStream.get());

		_shouldStream = true;
		
		while (_shouldStream) {
			soundio_wait_events(_soundio.get());
		}
	}

	void AudioInputOutput::StopAudioStreams()
	{
		_shouldStream = false;
		_inputStream = nullptr;
		_outputStream = nullptr;
		soundio_flush_events(_soundio.get());
	}

	int AudioInputOutput::GetDeviceCount(DeviceType deviceType) const {
		return deviceType == DeviceType::Input ? soundio_input_device_count(_soundio.get()) : soundio_output_device_count(_soundio.get());
	}

	SoundIoDevice* AudioInputOutput::GetDevice(DeviceType deviceType, int deviceIndex) const {
		return deviceType == DeviceType::Input ? soundio_get_input_device(_soundio.get(), deviceIndex) : soundio_get_output_device(_soundio.get(), deviceIndex);
	}

	std::vector<std::string> AudioInputOutput::GetAvailableDeviceNames(DeviceType deviceType, std::string currentDeviceName, int currentDeviceIndex) const {
		std::vector<std::string> deviceNames{};
		deviceNames.push_back(currentDeviceName);

		for (int i = 0; i < GetDeviceCount(deviceType); i++) {

			if (i != currentDeviceIndex) {
				const auto device = GetDevice(deviceType, i); 

				if (!device->is_raw) {
					deviceNames.push_back(std::string(device->name));
				}

				soundio_device_unref(device);
			}
		}

		return deviceNames;
	}

	void AudioInputOutput::SetDevice(DeviceType deviceType, const char* newDeviceName) {
		for (int i = 0; i < GetDeviceCount(deviceType); i++) {
			const auto device = GetDevice(deviceType, i);

			auto& currentDevice = deviceType == DeviceType::Input ? _inputDevice : _outputDevice;
			auto& currentDeviceIndex = deviceType == DeviceType::Input ? _inputDeviceIndex : _outputDeviceIndex;

			if (i != currentDeviceIndex && !device->is_raw && strcmp(device->name, newDeviceName) == 0) {
				currentDevice.reset(device);
				currentDeviceIndex = i;

				SetDeviceFormat(*currentDevice, deviceType);
				SetDeviceSampleRate(*currentDevice, deviceType);

				return;
			}

			soundio_device_unref(device);
		}
	}

	void AudioInputOutput::SetDeviceFormat(SoundIoDevice& device, DeviceType deviceType) {
		for (const auto& format : PrioritizedFormats) {
			if (soundio_device_supports_format(&device, format)) {
				if (deviceType == DeviceType::Input) {
					_inputFormat = format;
				}
				else {
					_outputFormat = format;
				}

				return;
			}
		}
	}

	void AudioInputOutput::SetDeviceSampleRate(SoundIoDevice& device, DeviceType deviceType) {
		for (const auto& sampleRate : PrioritizedSampleRates) {
			if (soundio_device_supports_sample_rate(&device, sampleRate)) {
				if (deviceType == DeviceType::Input) {
					_inputSampleRate = sampleRate;
				}
				else {
					_outputSampleRate = sampleRate;
				}

				return;
			}
		}
	}
}


