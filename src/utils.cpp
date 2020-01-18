#include "utils.hpp"

#include <vector>
#include <fstream>

//TODO: Reconsider 'val' parameter
NumValidity isFloatLiteral(const std::string &str, float &val) {
	try {
		std::size_t nRead;
		val = std::stof(str, &nRead);
		if(nRead != str.size() ) {
			return NumValidity::Invalid;
		}
	} catch(std::invalid_argument except ) {
		return NumValidity::Invalid;
	} catch(std::out_of_range except) {
		return NumValidity::Range;
	}
	return NumValidity::Ok;
}

//TODO: Reconsider 'val' parameter
NumValidity isIntLiteral(const std::string &str, int &val) {
	try {
		std::size_t nRead;
		val = std::stoi(str, &nRead);
		if(nRead != str.size() ) {
			return NumValidity::Invalid;
		}
	} catch(std::invalid_argument except ) {
		return NumValidity::Invalid;
	} catch(std::out_of_range except) {
		return NumValidity::Range;
	}
	return NumValidity::Ok;
}

std::string consumeFile(const char* path) {
	std::ifstream file;
	file.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	if(size < 1) return std::string();

	std::vector<char> bytes(size);
	file.read(bytes.data(), size);

	return std::string(bytes.data(), size);
}

bool endsWith(std::string_view sv, std::string_view end) {
	return sv.rfind(end) + end.size() == sv.size();
}

std::string getFileName(std::string_view sv) {
	auto index = sv.rfind('/');	//TODO: OS dependent(?)
	return index == std::string::npos ? std::string(sv) : std::string(sv.substr(index + 1, sv.size() ) );
}

std::string removeStem(std::string_view sv) {
	auto index = sv.rfind('.');
	return index == std::string::npos ? std::string(sv) : std::string(sv.substr(0, index) );
}
