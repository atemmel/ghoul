#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

enum struct TokenType {
	StringLiteral,      // ""
	IntLiteral,         // 5
	FloatLiteral,       // 15.7
	Terminator,			// \n

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
	Extern,				// extern

	Identifier,			// main, x, y, etc

	//Keep this one last
	NTokenTypes
};

constexpr static auto onelineComment = "//";

struct Token {
	TokenType type;
	std::string value;
	size_t index;
	size_t row;
	size_t col;

	constexpr static std::array<std::string_view, static_cast<size_t>(TokenType::NTokenTypes)> strings {
		"",		//String literal
		"",		//Int literal
		"",		//Float literal
		"\n",	//Statement termination

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
		"extern",

		""
	};

	constexpr static std::array<std::string_view, 4> altStrs = {
		"String literal",
		"Integer literal",
		"Floating point literal",
		"\\n"
	};

	constexpr static std::string_view getPrintString(const size_t index) {
		return index < altStrs.size() ? altStrs[index] : strings[index];
	}

	constexpr std::string_view getPrintString() const {
		return getPrintString(static_cast<size_t>(type) );
	}
};

using Tokens = std::vector<Token>;
using TokenIterator = Tokens::iterator;
using CTokenIterator = Tokens::const_iterator;
