#pragma once

#include <memory>
#include <functional>
#include <string>

#include "boost/lockfree/spsc_queue.hpp"
#include "miniaudio/miniaudio.h"

namespace Comms {

	enum class DeviceType { 
		Input, // Input devices such as a microphone
		Output // Output devices such as speakers
	};

	/*
	* Handles interaction with audio input (e.g. microphone) and output (e.g. speakers) devices.
	* This includes selecting specfic devices to read from and write to, as well as opening input and output streams to read and write the audio data.
	* Reading and writing data is done within the StartAudioStreams method which is intended to be done on a separate thread to the main application thread.
	* Audio data is stored in boost lockfree queues to prevent blocking the thread while waiting for access to the buffers for reading or writing data.
	* 
	* Audio device interaction is provided by the miniaudio library.
	*/
	class AudioInputOutput
	{

	public:
		/*
		* Constructor.
		*/
		AudioInputOutput();

		/*
		* @return The name of the input device if one has been selected, else nullptr.
		*/
		char* GetInputDeviceName() const;

		/*
		* @return The name of the output device if one has been selected, else nullptr.
		*/
		char* GetOutputDeviceName() const;

		/*
		* Selects the input device by name.
		* The user is generally presented a list of names of available input devices to choose from.
		* @see GetAvailableInputDeviceNames
		* 
		* @param inputDeviceName The name of the device being selected from the list of available devices.
		*/
		void SetInputDevice(const char* inputDeviceName);

		/*
		* Selects the ouput device by name.
		* The user is generally presented a list of names of available output devices to choose from.
		* @see GetAvailableOutputDeviceNames
		*
		* @param outputDeviceName The name of the device being selected from the list of available devices.
		*/
		void SetOutputDevice(const char* outputDeviceName);

		/*
		* @return The list of available input device names.
		*/
		std::vector<std::string> GetAvailableInputDeviceNames() const;

		/*
		* @return The list of available output device names.
		*/
		std::vector<std::string> GetAvailableOutputDeviceNames() const;

		/*
		* Begins reading from the input device via a input stream, and writing to the output device via an output stream.
		* Data read from the input device stream is written to a non-locking single producer single consumer queue.
		* Data to write to the output stream is read from a non-locking single producer single consumer queue.
		* Data is read and written as bytes, representing audio samples in a format and frequency dependent on the capabilities of the devices.
		* This function should be run on an independent thread.
		* 
		* @param inputData The queue used to store data from the input device.
		* @param outputData The queue to used to retrieve data to write to the output device.
		*/
		void StartAudioStreams(std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> inputData,
			std::shared_ptr<boost::lockfree::spsc_queue<std::int16_t, boost::lockfree::capacity<262144>>> outputData);

		/*
		* Signals streaming of the audio data to end and destorys the streams.
		*/
		void StopAudioStreams();

	private:

		/*
		* @param deviceType The type of device to query the availabile device count for.
		* @return The number of available input or output devices.
		*/
		int GetDeviceCount(DeviceType deviceType) const;

		/*
		* Retrieves a representation of an audio device from the underlying sound subsystem (WASAPI).
		* 
		* @param deviceType The type of device to retrieve.
		* @param deviceIndex The index of the device to retrieve.
		* @return The retrieved device.
		*/
		void GetDevice(DeviceType deviceType, int deviceIndex) const;

		/*
		* Retrieves a list of available devices, beginning with the currently selected device.
		* 
		* @param deviceType The type of device to include in the list.
		* @param currentDeviceName The name of the current device. This is placed first in the list.
		* @param currentDeviceIndex The index of the current device.
		* @return The list of available device names.
		*/
		std::vector<std::string> GetAvailableDeviceNames(DeviceType deviceType, std::string currentDeviceName, int currentDeviceIndex) const;

		/*
		* Sets a currently selected device.
		* 
		* @param deviceType Type of the device to set.
		* @param newDeviceName Name of the device to set.
		*/
		void SetDevice(DeviceType deviceType, const char* newDeviceName);
	};
}


