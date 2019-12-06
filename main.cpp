#include <string_view>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <vector>
#include <array>

#ifdef DEBUG
#define verboseAssert(condition, strv) \
	assert(condition && strv);
#else
#define verboseAssert(condition, strv)
#endif

enum TokenType {
	StringLiteral,      // ""
	IntLiteral,         // 5
	FloatLiteral,       // 15.7

	ParensOpen,         // (
	ParensClose,        // )
	BlockOpen,          // {
	BlockClose,         // }
	DynamicArrayStart,  // [
	DynamicArrayEnd,    // ]
	StaticArrayStart,   // [[
	StaticArrayEnd,     // ]]

	Add,                // +
	Subtract,           // -
	Multiply,           // *
	Divide,             // /
	And,                // &
	Or,                 // |
	Xor,                // ^
	Modulo,             // %
	Less,               // <
	Greater,            // >
	Assign,				// =
	
	Equivalence,        // ==
	NotEquivalence,     // !=
	AddEquals,          // +=
	SubtractEquals,     // -=
	MutiplyEquals,      // *=
	DivideEquals,       // /=
	AndEquals,          // &=
	OrEquals,           // |=
	XorEquals,          // ^=
	ModuloEquals,       // %=
	LessEquals,         // <=
	GreaterEquals,      // >=
	
	AddressOf,          // &
	Dereference,        // *
	Inverse,            // !
	Negate,             // -

	Ternary,            // ?

	//Reserved words
	Function,           // fn
	While,              // while
	For,                // for
	If,                 // if
	Else,               // else
	ElseIf,             // else if
	True,               // true
	False,              // false
	Null,               // null
	Struct,	            // struct

	Identifier,			// main, x, y, etc

	//Keep this one last
	NTokenTypes
};

constexpr static std::array<std::string_view, TokenType::NTokenTypes> tokenStrings {
	"",		//String literal
	"",		//Int literal
	"",		//Float literal
	"(",
	")",
	"{",
	"}",
	"[",
	"]",
	"[[",
	"]]",
	"+",
	"-",
	"*",
	"/",
	"&",
	"|",
	"^",
	"%",
	"<",
	">",
	"=",
	"==",
	"!=",
	"+=",
	"-=",
	"*=",
	"/=",
	"&=",
	"|=",
	"^=",
	"%=",
	"<=",
	">=",
	"&",
	"*",
	"!",
	"-",
	"?",

	"fn",
	"while",
	"for",
	"if",
	"else",
	"else if",
	"true",
	"false",
	"null",
	"struct",

	""
};

enum NumValidity {
	Ok,
	Invalid,
	Range
};

struct Token {
	TokenType type;
	std::string value;
};

using Tokens = std::vector<Token>;

//TODO: Reconsider 'val' parameter
NumValidity isFloatLiteral(const std::string &str, float &val) {
	try {
		std::size_t nRead;
		val = std::stof(str, &nRead);
		if(nRead != str.size() ) {
			return NumValidity::Invalid;
		}
	} catch(std::invalid_argument except ) {
		return NumValidity::Invalid;
	} catch(std::out_of_range except) {
		return NumValidity::Range;
	}
	return NumValidity::Ok;
}

//TODO: Reconsider 'val' parameter
NumValidity isIntLiteral(const std::string &str, int &val) {
	try {
		std::size_t nRead;
		val = std::stoi(str, &nRead);
		if(nRead != str.size() ) {
			return NumValidity::Invalid;
		}
	} catch(std::invalid_argument execpt ) {
		return NumValidity::Invalid;
	} catch(std::out_of_range except) {
		return NumValidity::Range;
	}
	return NumValidity::Ok;
}

std::string consumeFile(const char* path) {
	std::ifstream file;
	file.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> bytes(size);
	file.read(bytes.data(), size);

	return std::string(bytes.data(), size);
}

void displaySource(const std::string &str) {
	int line = 1;
	int column = 1;
	int maxLine = 1;
	int maxColumn = 0;
	std::string lineStr;
	std::string columnStr;

	for(const auto c : str) {
		if(c == '\n') {
			++maxLine;
			column = 0;
		}
		++column;

		if(column > maxColumn) {
			maxColumn = column;
		}
	}

	column = 1;
	auto lineDigits = std::to_string(maxLine).size();
	auto columnDigits = std::to_string(maxColumn).size();

	auto pad = [](const size_t n) {
		for(size_t i = 0; i < n; i++) {
			std::cout << ' ';
		}
	};

	for(const auto c : str) {
		lineStr = std::to_string(line);
		columnStr = std::to_string(column);
		std::cout << "L: ";
		pad(lineDigits - lineStr.size() );
		std::cout << lineStr;
		std::cout << " C: ";
		pad(columnDigits - columnStr.size() ); 
		std::cout << columnStr << ' ';

		if(c == '\n') {
			std::cout << "'\\n'\n";
			++line;
			column = 0;
		} else if(c == ' ') {
			std::cout << "' '\n";
		} else if(c == '\t') {
			std::cout << "'\\t'\n";
		} else {
			std::cout << '\'' << c << '\'' << '\n';
		}

		++column;
	}
}

template<typename ForwardIterator>
std::string lexStrLiteral(ForwardIterator begin, ForwardIterator end) {
	return std::string(std::next(begin), std::prev(std::find_if(std::next(begin), end, [](const char c) {
		return c == '"';
	})));
}

int lexToken(const std::string &str) {
	int i = 0;
	for(; i < static_cast<int>(TokenType::NTokenTypes); i++) {
		if(str == tokenStrings[i]) break;
	}
	return i;
}

Tokens lexTokens(const std::string &str) {
	Tokens tokens;
	auto it = str.cbegin();
	auto start = it;
	auto end = str.cend();
	Token current;

	int idummy;
	float fdummy;
	NumValidity validity;

SEEK_NEXT_TOKEN:
	if(std::isspace(*it) ) {
		it++;
		if(it == end) goto DONE;
		else goto SEEK_NEXT_TOKEN;
	}

LEX_TOKEN:
STRING_LITERAL_TEST:
	if(*it == '"') {	
		current.type = TokenType::StringLiteral;
		current.value = lexStrLiteral(it, end);
		goto INSERT_TOKEN;
	}

SEEK_TOKEN_END:
	start = it;
	if(isalnum(*it) ) {
		while(!std::isspace(*it) && isalnum(*it) ) {
			if(++it == end) {
				--it;
				goto TOKEN_TEST;
			}
		}
	} else {
		while(!std::isspace(*it) && !isalnum(*it) ) {
			if(++it == end) {
				--it;
				goto TOKEN_TEST;
			}
			if(int index = lexToken(std::string(start, it) ); index != static_cast<int>(TokenType::NTokenTypes) ) {
				current.value = tokenStrings[index];
				current.type = static_cast<TokenType>(index);
				goto INSERT_TOKEN;
			}
		}
	}

TOKEN_TEST:
	current.value = std::move(std::string(start, it) );
	for(int i = 0; i < static_cast<int>(TokenType::NTokenTypes); i++) {
		if(current.value == tokenStrings[i]) {
			current.type = static_cast<TokenType>(i);
			goto INSERT_TOKEN;
		}
	}

INTEGER_TEST:
	validity = isIntLiteral(current.value, idummy);
	if(validity == NumValidity::Ok) {
		current.type = TokenType::IntLiteral;
		goto INSERT_TOKEN;
	} else {	//TODO: Add error handling
	}

FLOAT_TEST:
	validity = isFloatLiteral(current.value, fdummy);
	if(validity == NumValidity::Ok) {
		current.type == TokenType::FloatLiteral;
		goto INSERT_TOKEN;
	} else {	//TODO: Add error handling
	}

IDENTIFIER_TOKEN:
	current.type = TokenType::Identifier;

INSERT_TOKEN:
	tokens.push_back(current);
	goto SEEK_NEXT_TOKEN;

DONE:
	return tokens;
}

void displayTokens(const Tokens &tokens) {
	for(const Token &token : tokens) {
		std::cout << "Value: " << token.value << "\nType: " 
			<< static_cast<int>(token.type) << "\n\n";
	}
}

int main() {
#ifdef DEBUG
	float f;
	verboseAssert(isFloatLiteral("127.5", f) == NumValidity::Ok,      "Ok test failed");
	verboseAssert(isFloatLiteral("127r5", f) == NumValidity::Invalid, "Invalid test failed");
	verboseAssert(isFloatLiteral("1e700", f) == NumValidity::Range,   "Range test failed");

	int i;
	verboseAssert(isIntLiteral("12345", i) == NumValidity::Ok,      "Ok test failed");
	verboseAssert(isIntLiteral("123.4", i) == NumValidity::Invalid, "Invalid test failed");
	verboseAssert(isIntLiteral("5000000000", i) == NumValidity::Range,   "Range test failed");
	std::cout << "All assertions passed\n";
#endif

	auto str = consumeFile("main.scp");
	displaySource(str);

	auto tokens = lexTokens(str);
	displayTokens(tokens);
}
