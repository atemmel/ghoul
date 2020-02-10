#include "errstack.hpp"
#include "utils.hpp"

void ErrorStack::setFile(std::string *ptr) {
	verboseAssert(ptr, "Empty pointer passed into function");
	file = ptr;
}

bool ErrorStack::empty() const {
	return stack.empty();
}

void ErrorStack::push(const std::string &str, Token *token) {
	stack.push_back({str, token});
}

void ErrorStack::unwind() const {
	for(const Error &error : stack) {
		if(error.token) {
			std::cerr << *file << ':' << error.token->row << ':' << error.token->col 
				<<   '\n' << error.str << '\n';
		} else {
			std::cerr << *file <<   '\n' << error.str << '\n';
		}
	}
}
