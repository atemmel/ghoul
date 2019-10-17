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

enum Type {
	StringLiteral,      // ""
	IntLiteral,         // 5
	FloatLiteral,       // 15.7

	While,              // while
	For,                // for
	If,                 // if
	Else,               // else
	ElseIf,             // else if
	True,               // true
	False,              // false
	Null,               // null
	Struct,	            // struct

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

	Ternary             // ?
};

enum NumValidity {
	Ok,
	Invalid,
	Range
};

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
}
