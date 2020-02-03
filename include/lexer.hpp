#include "token.hpp"
#include "utils.hpp"

#include <algorithm>
#include <string> 

//TODO: Improve and specify this implementation to find escape chars
template<typename ForwardIterator>
std::string lexStrLiteral(ForwardIterator begin, ForwardIterator end) {
	return std::string(begin, std::prev(std::find_if(begin, end, [](const char c) {
		return c == '"';
	})));
}

class Lexer {
public:
	Tokens &&lexTokens(const std::string &str);
private:
	int step();
	int lexToken(const std::string &str) const;
	Tokens tokens;
	std::string::const_iterator iterator;
	size_t row = 1;
	size_t col = 1;
};
