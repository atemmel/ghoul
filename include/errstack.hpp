#pragma once
#include "token.hpp"

#include <ostream>
#include <string>
#include <vector>

class ErrorStack {
public:
	void push(const std::string &str, const Token &token);
private:
};
