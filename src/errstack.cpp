#include "errstack.hpp"
#include "utils.hpp"

void ErrorStack::setFile(std::string *ptr) {
	verboseAssert(ptr, "Empty pointer passed into function");
	file = ptr;
}

bool ErrorStack::empty() const {
	return stack.empty();
}

void ErrorStack::push(const std::string &str, const Token &token) {
	stack.push_back({str, token});
}

void ErrorStack::unwind() const {
	for(const Error &error : stack) {
		std::cerr << *file << ':' << error.token.row << ':' << error.token.col 
			<<  ':' << error.token.index << '\n' << error.str << '\n';
	}
}
