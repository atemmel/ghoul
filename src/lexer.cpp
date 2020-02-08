#include "errstack.hpp"
#include "global.hpp"
#include "lexer.hpp"

#include <iostream>

Tokens &&Lexer::lexTokens(const std::string &str) {
	const auto begin = str.cbegin();
	const auto end = str.cend();
	iterator = str.cbegin();
	auto start = iterator;
	Token current;
	size_t row = 1;
	size_t col = 1;

	int idummy;
	float fdummy;
	NumValidity validity;

SEEK_NEXT_TOKEN:
	if(iterator == end) goto DONE;
	if(std::isspace(*iterator) ) {
		if(step() == '\n') {
			//Discard consecutive terminator tokens
			if(current.type == TokenType::Terminator) {	
				goto SEEK_NEXT_TOKEN;
			}
			current.value = "\\n";
			current.type = TokenType::Terminator;
			goto INSERT_TOKEN;
		}
		if(iterator == end) goto DONE;
		else goto SEEK_NEXT_TOKEN;
	}
	else if(auto next = std::next(iterator); next != end ) {
		//Detect single line comments and skip parsing them
		std::string sdummy(iterator, std::next(next) );
		if(sdummy == onelineComment) {
			while(iterator != end && step() != '\n');
			goto SEEK_NEXT_TOKEN;
		}
	}

LEX_TOKEN:
STRING_LITERAL_TEST:	//Test if token is string literal
	if(*iterator == '"') {	
		current.col = col;
		current.row = row;
		start = std::next(iterator);
		//TODO: Account for step
		char prev = 0;
		iterator = std::find_if(start, end, [&](const char c) {
			if(prev == '\\') {
				prev = c;
				return false;
			}
			prev = c;
			return c == '"';
		});

		if(iterator == end) {	//TODO: Add error handling
			goto DONE;
		}

		++iterator;

		current.type = TokenType::StringLiteral;
		current.value = std::string(start, iterator - 1);
		expand(current.value);
		goto INSERT_TOKEN;
	}

SEEK_TOKEN_END:	//Iterate until end of token
	start = iterator;
	if(isalnum(*iterator) ) {
		//TODO: Account for step
		while(!std::isspace(*iterator) && isalnum(*iterator) ) {
			if(++iterator == end) {
				--iterator;
				goto TOKEN_TEST;
			}
		}
	} else {
		while(!std::isspace(*iterator) && !isalnum(*iterator) ) {
			if(++iterator == end) {
				--iterator;
				goto TOKEN_TEST;
			}
			if(int index = lexToken(std::string(start, iterator) ); index != static_cast<int>(TokenType::NTokenTypes) ) {
				current.value = Token::strings[index];
				current.type = static_cast<TokenType>(index);
				goto INSERT_TOKEN;
			}
		}
	}

TOKEN_TEST:	//Test if token is valid token
	current.value = std::move(std::string(start, iterator) );
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
	if(std::all_of(start, iterator, ::isalnum) ) {
		current.type = TokenType::Identifier;
	} else {
		//TODO: Log error regarding unknown token
		Global::errStack.push("Unrecognized token", current);
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

int Lexer::step() {
	if(*iterator == '\n') {
		row++;
		col = 1;
	} else col++;
	return *iterator++;
}

int Lexer::get() const {
	return *iterator;
}

void Lexer::expand(std::string &str) {
	for(auto it = std::find(str.begin(), str.end(), '\\');
			it != str.end(); it = std::find(it, str.end(), '\\') ) {
		str.erase(it);
		switch(*it) {
			case 'n':
				*it = '\n';
				break;
			case 't':
				*it = '\t';
				break;
			case '"':
				// :)
				break;
			default:
				Global::errStack.push(std::string("Unrecognized escape char \\")
						+ *it, Token() );
		}
	}

}

int Lexer::lexToken(const std::string &str) const {
	int i = 0;
	for(; i < static_cast<int>(TokenType::NTokenTypes); i++) {
		if(str == Token::strings[i]) break;
	}
	return i;
}
