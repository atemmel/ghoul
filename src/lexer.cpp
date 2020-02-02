#include "lexer.hpp"

#include <iostream>

Tokens &&Lexer::lexTokens(const std::string &str) {
	auto it = str.cbegin();
	auto start = it;
	const auto begin = str.cbegin();
	const auto end = str.cend();
	Token current;
	size_t row = 1;
	size_t col = 1;

	int idummy;
	float fdummy;
	NumValidity validity;

SEEK_NEXT_TOKEN:
	if(it == end) goto DONE;
	if(std::isspace(*it) ) {
		if(*it++ == '\n') {
			//Discard consecutive terminator tokens
			if(current.type == TokenType::Terminator) {	
				goto SEEK_NEXT_TOKEN;
			}
			current.value = "\\n";
			current.type = TokenType::Terminator;
			goto INSERT_TOKEN;
		}
		if(it == end) goto DONE;
		else goto SEEK_NEXT_TOKEN;
	}
	else if(auto next = std::next(it); next != end ) {
		//Detect single line comments and skip parsing them
		std::string sdummy(it, std::next(next) );
		if(sdummy == onelineComment) {
			for(; it != end && *it != '\n'; it++);
			goto SEEK_NEXT_TOKEN;
		}
	}

LEX_TOKEN:
STRING_LITERAL_TEST:	//Test if token is string literal
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

SEEK_TOKEN_END:	//Iterate until end of token
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
				current.value = Token::strings[index];
				current.type = static_cast<TokenType>(index);
				goto INSERT_TOKEN;
			}
		}
	}

TOKEN_TEST:	//Test if token is valid token
	current.value = std::move(std::string(start, it) );
	for(int i = 0; i < static_cast<int>(TokenType::NTokenTypes); i++) {
		if(current.value == Token::strings[i]) {
			current.type = static_cast<TokenType>(i);
			goto INSERT_TOKEN;
		}
	}

INTEGER_TEST:	//Test if token is integer literal
	validity = isIntLiteral(current.value, idummy);
	if(validity == NumValidity::Ok) {
		current.type = TokenType::IntLiteral;
		goto INSERT_TOKEN;
	} else {	//TODO: Add error handling
	}

FLOAT_TEST:	//Test if token is floating point literal
	validity = isFloatLiteral(current.value, fdummy);
	if(validity == NumValidity::Ok) {
		current.type = TokenType::FloatLiteral;
		goto INSERT_TOKEN;
	} else {	//TODO: Add error handling
	}

IDENTIFIER_TOKEN:	//Otherwise, must be an identifier
	if(std::all_of(start, it, ::isalnum) ) {
		current.type = TokenType::Identifier;
	} else {
		//TODO: Log error regarding unknown token
		std::cerr << "Unrecognized token: " << current.value << '\n';
		goto SEEK_NEXT_TOKEN;
	}

INSERT_TOKEN:
	current.index = std::distance(begin, start);
	tokens.push_back(current);
	current.value.clear();
	goto SEEK_NEXT_TOKEN;

DONE:
	return std::move(tokens);
}

int Lexer::lexToken(const std::string &str) const {
	int i = 0;
	for(; i < static_cast<int>(TokenType::NTokenTypes); i++) {
		if(str == Token::strings[i]) break;
	}
	return i;
}
