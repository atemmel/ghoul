#pragma once
#include "token.hpp"

#include <iostream>
#include <string>
#include <vector>

class ErrorStack {
public:
	void setFile(std::string *ptr);
	bool empty() const;
	void push(const std::string &str, Token *token);
	void unwind() const;
private:
	struct Error {
		std::string str;
		Token *token;
	};

	std::string *file = nullptr;
	std::vector<Error> stack;
};
