#include "token.hpp"
#include "utils.hpp"

#include <algorithm>
#include <string> 
#include <vector>

class Lexer {
public:
	Tokens &&lexTokens(const std::string &str);
private:
	void index(const std::string &str);
	int step();
	int get() const;
	void expand(std::string &str);

	Tokens tokens;
	std::vector<size_t> rows;
	std::vector<size_t> cols;
	std::string::const_iterator iterator;
};
