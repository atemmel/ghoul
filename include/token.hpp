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
	Identifier,			// main, x, y, etc
	Variadic,			// ...
	Member,				// .
	Comma,				// ,

	ParensOpen,         // (
	ParensClose,        // )
	BlockOpen,          // {
	BlockClose,         // }
	ArrayStart,  		// [
	ArrayEnd,    		// ]

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
	
	Inverse,            // !

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
	Return,				// return
	Link,				// link


	//Keep this one last
	NTokenTypes
};

constexpr static auto onelineComment = "//";

struct Token {
	TokenType type;
	std::string value;
	size_t index;
	size_t row;		//TODO: Implement this
	size_t col;		//TODO: Implement this

	constexpr static std::array<std::string_view, static_cast<size_t>(TokenType::NTokenTypes)> strings {
		"",		//String literal
		"",		//Int literal
		"",		//Float literal
		"\n",	//Statement termination
		"",
		"...",
		".",
		",",

		"(",
		")",
		"{",
		"}",
		"[",
		"]",
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
		"!",
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
		"return",
		"link"
	};

	constexpr static std::array<std::string_view, 5> altStrs = {
		"String literal",
		"Integer literal",
		"Floating point literal",
		"\\n",
		"Identifier"
	};

	constexpr static std::string_view getPrintString(const size_t index) {
		return index < altStrs.size() ? altStrs[index] : strings[index];
	}

	constexpr std::string_view getPrintString() const {
		return getPrintString(static_cast<size_t>(type) );
	}

	constexpr static int precedence(TokenType type) {
		switch(type) {
			case TokenType::Multiply:
				return 2;
			case TokenType::Add:
			case TokenType::Subtract:
				return 1;
			case TokenType::Assign:
				return 0;
			default:
				return -1;
		}
	}

	constexpr int precedence() const {
		return Token::precedence(type);
	}
};

using Tokens = std::vector<Token>;
using TokenIterator = Tokens::iterator;
using CTokenIterator = Tokens::const_iterator;
