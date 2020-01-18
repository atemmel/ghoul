#pragma once
#include <string>
#include <cassert>
#include <string_view>

#ifdef DEBUG
#define verboseAssert(condition, strv) \
	assert(condition && strv);
#else
#define verboseAssert(condition, strv)
#endif

enum struct NumValidity {
	Ok,
	Invalid,
	Range
};

//TODO: Reconsider 'val' parameter
NumValidity isFloatLiteral(const std::string &str, float &val);

//TODO: Reconsider 'val' parameter
NumValidity isIntLiteral(const std::string &str, int &val);

std::string consumeFile(const char* path);

bool endsWith(std::string_view sv, std::string_view end);

std::string getFileName(std::string_view sv);

std::string removeStem(std::string_view sv);
