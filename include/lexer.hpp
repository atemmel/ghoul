#include "token.hpp"
#include "utils.hpp"

#include <algorithm>
#include <string> 

class Lexer {
public:
	Tokens &&lexTokens(const std::string &str);
private:
	int step();
	int get() const;
	void expand(std::string &str);
	int lexToken(const std::string &str) const;
	Tokens tokens;
	std::string::const_iterator iterator;
	size_t row = 1;
	size_t col = 1;
};
