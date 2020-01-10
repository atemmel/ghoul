#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

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

constexpr static auto onelineComment = "//";

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

struct Token {
	TokenType type;
	std::string value;
};

using Tokens = std::vector<Token>;
using TokenIterator = Tokens::iterator;
using CTokenIterator = Tokens::const_iterator;
