#pragma once
#include "token.hpp"

#include <ostream>
#include <string>
#include <vector>

class ErrorStack {
public:
	bool empty() const;
	void push(const std::string &str, const Token &token);
	void unwind();
private:
	struct Error {
		std::string str;
		Token token;
	};
	std::vector<Error> stack;
};
