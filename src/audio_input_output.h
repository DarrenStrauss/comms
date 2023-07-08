#pragma once

#include <memory>
#include <string>

#include "boost/lockfree/spsc_queue.hpp"
#include "miniaudio/miniaudio.h"

namespace Comms {

	/*
	* Handles interaction with audio input (e.g. microphone) and output (e.g. speakers) devices.
	* This includes listing available audio devices and selecting desired devices to read from and write to.
	* Audio data is stored in boost lockfree queues to prevent blocking the thread while waiting for access to the buffers for reading or writing data.
	* 
	* Audio device interaction is provided by the miniaudio library.
	*/
	class AudioInputOutput
	{

	public:
		/*
		* Constructor.
		* 
		* @param inputBuffer Lockfree queue to write input audio data to.
		* @param outputBuffer Lockfree queue to read output audio data from.
		*/
		AudioInputOutput(std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> inputBuffer,
			std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> outputBuffer);

		/*
		* @return The name of the input device if one has been selected, else empty string.
		*/
		std::string GetInputDeviceName() const;

		/*
		* @return The name of the output device if one has been selected, else empty string.
		*/
		std::string GetOutputDeviceName() const;

		/*
		* Selects the input device by name.
		* The user is presented a list of names of available input devices to choose from.
		* @see GetInputDeviceNames
		* 
		* @param inputDeviceName The name of the device being selected from the list of available devices.
		*/
		void SetInputDevice(std::string inputDeviceName);

		/*
		* Selects the ouput device by name.
		* The user is presented a list of names of available output devices to choose from.
		* @see GetOutputDeviceNames.
		*
		* @param outputDeviceName The name of the device being selected from the list of available devices.
		*/
		void SetOutputDevice(std::string outputDeviceName);

		/*
		* @return The list of available input device names.
		*/
		std::vector<std::string> GetInputDeviceNames() const;

		/*
		* @return The list of available output device names.
		*/
		std::vector<std::string> GetOutputDeviceNames() const;

		/*
		* Begins reading from the input device and writing to the output device.
		*/
		void StartAudioStreams();

		/*
		* Stops reading from the input device and writing to the output device.
		*/
		void StopAudioStreams();

	private:

		/*
		* Convenience structure to simplify iterating over available devices.
		*/
		struct Device {
			std::string _name; // Device name shown to the user.
			ma_device_id _id; // Underlying device id used by the WASAPI backend.
			bool _isInput; // Whether or not the device is an input device (as opposed to output).
		};

		/*
		* @return A list of all audio devices available on the system.
		*/
		std::vector<Device> GetDevices() const;

		/*
		* Function to read data from the input device into the input buffer.
		* Used as a callback and is called when there is audio data available to be read.
		* 
		* @param device The input device.
		* @param output Not used.
		* @param input Pointer to the audio samples that can be read into the buffer.
		* @param numFrames Number of audio samples that can be read.
		*/
		static void ReadFromDevice(ma_device* device, void* output, const void* input, ma_uint32 numFrames);

		/*
		* Function to write data from the output buffer to the output device.
		* Used as a callback and is called when the output device is ready to recieve data.
		* If there is data available in the output buffer, silence (zeros) will be provided to the device.
		*
		* @param device The output device.
		* @param output Pointer to the memory to write audio samples to so that they can be provided to the device.
		* @param input Not used.
		* @param numFrames Number of audio samples that can be recieved by the device.
		*/
		static void WriteToDevice(ma_device* device, void* output, const void* input, ma_uint32 numFrames);

		std::unique_ptr<ma_context, std::function<void(ma_context*)>> _audioContext; // MiniAudio context. This represents the WASAPI backend.
		std::unique_ptr<ma_device, std::function<void(ma_device*)>> _inputDevice = nullptr; // Input device.
		std::unique_ptr<ma_device, std::function<void(ma_device*)>> _outputDevice = nullptr; // Output device.
		std::string _inputDeviceName = ""; // User readable name for the input device to be shown on the UI.
		std::string _outputDeviceName = ""; // User readable name for the output device to be shown on the UI.
		std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> _inputBuffer; // Lockfree queue to store input audio data.
		std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> _outputBuffer; // Lockfree queue to store output audio data.
	};
}


