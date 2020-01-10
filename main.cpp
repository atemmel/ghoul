#include <string_view>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <vector>
#include <array>

#include "clock.hpp"
#include "llvm.hpp"
#include "token.hpp"

#ifdef DEBUG
#define verboseAssert(condition, strv) \
	assert(condition && strv);
#else
#define verboseAssert(condition, strv)
#endif

enum NumValidity {
	Ok,
	Invalid,
	Range
};

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

	if(size < 1) return std::string();

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
	return std::string(begin, std::prev(std::find_if(begin, end, [](const char c) {
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
	const auto begin = str.cbegin();
	const auto end = str.cend();
	Token current;

	int idummy;
	float fdummy;
	NumValidity validity;

SEEK_NEXT_TOKEN:
	if(it == end) goto DONE;
	if(std::isspace(*it) ) {
		it++;
		if(it == end) goto DONE;
		else goto SEEK_NEXT_TOKEN;
	}
	else if(auto next = std::next(it); next != end ) {
		std::string sdummy(it, std::next(next) );
		if(sdummy == onelineComment) {
			for(; it != end && *it != '\n'; it++);
			goto SEEK_NEXT_TOKEN;
		}
	}

LEX_TOKEN:
STRING_LITERAL_TEST:
	if(*it == '"') {	
		start = std::next(it);
		it = std::find_if(start, end, [](const char c) {
			return c == '"';
		});

		if(it == end) {	//TODO: Add error handling
			goto DONE;
		}

		++it;

		current.type = TokenType::StringLiteral;
		current.value = std::string(start, it - 1);
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

	ModuleInfo mi {
		"main",
		"main.scp",
		"main.o"
	};

	static Context ctx;

	float time;
	Clock clock;
	auto str = consumeFile(mi.fileName.c_str() );

	time = clock.get();
	std::cout << mi.fileName.c_str() << " read in " << time << " s\n";
	//displaySource(str);

	clock.restart();
	auto tokens = lexTokens(str);
	time = clock.get();
	std::cout << mi.fileName.c_str() << " tokenized in " << time << " s\n";
	displayTokens(tokens);

	clock.restart();
	mi.ast.buildTree(tokens);
	time = clock.get();
	std::cout << mi.fileName.c_str() << " ast built in " << time << " s\n";
	displayTokens(tokens);

	clock.restart();
	gen(&mi, &ctx);
	std::cout << mi.objName.c_str() << " object file built in " << time << " s\n";
}
