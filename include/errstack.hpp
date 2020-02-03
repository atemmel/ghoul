#pragma once
#include "token.hpp"

#include <iostream>
#include <string>
#include <vector>

class ErrorStack {
public:
	bool empty() const;
	void push(const std::string &file, const std::string &str, const Token &token);
	void unwind();
private:
	struct Error {
		std::string file;
		std::string str;
		Token token;
	};

	std::vector<Error> stack;
};
