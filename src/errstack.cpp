#include "errstack.hpp"

bool ErrorStack::empty() const {
	return stack.empty();
}

void ErrorStack::push(const std::string &file, const std::string &str, const Token &token) {
	stack.push_back({file, str, token});
}

void ErrorStack::unwind() {
	for(const Error &error : stack) {
		std::cout << error.file << '\n'
			<< error.token.row << ':' << error.token.col << ' ' 
			<< error.str << '\n';
	}
}
