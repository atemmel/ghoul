#include <string_view>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
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
}
