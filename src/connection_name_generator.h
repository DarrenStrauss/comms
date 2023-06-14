#pragma once

#include <random>

#include "boost\iostreams\device\mapped_file.hpp"

namespace Comms {
	/*
	* Generates a name that can be used to identify a WebRTC connection.
	* Names are constructed from two randomly selected words from a list of 5025 frequently used English words.
	* The word list is sourced from www.wordfrequency.info
	* and cross referenced against the list of single words from the Moby Word List project (www.archive.org/details/mobywordlists03201gut) to remove hyphenated words.
	* There are 12622800 possible unique names.
	* For 2 generations, the probability of collision is 0.00000792%.
	* For 100 generations, the probability of collision is 0.0037%.
	* For 1000 generations, the probably of collision is 72.3%.
	* Therefore this generation method should be improved if simultaneous user count is expected to exceed 100 users.
	*/
	class ConnectionNameGenerator {

	public:
		/*
		* Constructor
		*/
		ConnectionNameGenerator();

		/*
		* Generates and returns a human-readable name to be used to establish a WebRTC connection.
		* Session names are comprised of two English words.
		*/
		std::string GenerateConnectionName();

	private:
		/*
		* Given an offset representing a position of a character in the word list file,
		* returns the word at position.
		*/
		std::string GetWordAtOffset(std::size_t offset);

		boost::iostreams::mapped_file_source _wordFile; // Memory mapped file containing the list of words. The file is memory mapped for efficient random access.
		std::mt19937 _randomNumberGenerator; // Random number generator used to randomly select words from the list.
		std::uniform_int_distribution<std::size_t> _randomNumberDistribution; // Distribution used to select random numbers.
	};
}
