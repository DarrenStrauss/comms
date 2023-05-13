#include "connection_name_generator.h"

#include <sstream>

namespace {
	constexpr const char* WordFileName = "words_en.txt";
}

namespace Comms {
	ConnectionNameGenerator::ConnectionNameGenerator() :
		_wordFile(WordFileName),
		_randomNumberGenerator(std::random_device{}()),
		_randomNumberDistribution(54, _wordFile.size()) { // First 53 characters used to cite source of word list
	}

	std::string ConnectionNameGenerator::GenerateConnectionName() {
		std::stringstream stream("");

		stream << GetWordAtOffset(_randomNumberDistribution(_randomNumberGenerator));
		stream << " ";
		stream << GetWordAtOffset(_randomNumberDistribution(_randomNumberGenerator));

		return stream.str();
	}

	std::string ConnectionNameGenerator::GetWordAtOffset(std::size_t offset) {
		const char* wordStart = _wordFile.data() + offset;

		while (wordStart > _wordFile.data() && *(wordStart - 1) != '\n') {
			wordStart--;
		}

		const char* wordEnd = _wordFile.data() + offset;

		while (wordEnd < (_wordFile.data() + _wordFile.size()) && (*wordEnd != '\r' && *wordEnd != '\n')) {
			wordEnd++;
		}

		std::string word(wordStart, wordEnd);
		word[0] = std::toupper(word[0]);

		return word;
	}
}
