#pragma once
#include <cppcoro/generator.hpp>
#include <iostream>
#include <charconv>
enum class TokenType : char {
	STRING,
	KEYWORD,
	SEMICOLON,
	COMMENT,
	EQUALS,
	FLOAT_LITERAL,
	INT_LITERAL,
	COLON,
	CURLY_BRACKET_OPEN,
	CURLY_BRACKET_CLOSE,
	SQUARE_BRACKET_OPEN,
	SQUARE_BRACKET_CLOSE,
	NONE
}; 

std::string to_string(TokenType ttype) {
	switch (ttype)
	{
	case TokenType::STRING:
		return "STRING";
		break;
	case TokenType::KEYWORD:
		return "KEYWORD";
		break;
	case TokenType::SEMICOLON:
		return "SEMICOLON";
		break;
	case TokenType::COMMENT:
		return "COMMENT";
		break;
	case TokenType::EQUALS:
		return "EQUALS";
		break;
	case TokenType::FLOAT_LITERAL:
		return "FLOAT";
		break;
	case TokenType::INT_LITERAL:
		return "INT";
		break;
	case TokenType::COLON:
		return "COLON";
		break;
	case TokenType::CURLY_BRACKET_OPEN:
		return "CURLY BRACKET OPEN";
		break;
	case TokenType::CURLY_BRACKET_CLOSE:
		return "CURLY BRACKET CLOSE";
		break;
	case TokenType::SQUARE_BRACKET_OPEN:
		return "SQUARE BRACKET OPEN";
		break;
	case TokenType::SQUARE_BRACKET_CLOSE:
		return "SQUARE BRACKET CLOSE";
		break;



	case TokenType::NONE:
		return "NONE";
		break;
	default:
		return "ERROR";
		break;
	}
}

enum class CharacterType : char {
	SPACE,
	CURLY_BRACKET_OPEN,
	CURLY_BRACKET_CLOSE,
	SQUARE_BRACKET_OPEN,
	SQUARE_BRACKET_CLOSE,
	LETTER,
	DIGIT,
	SLASH,
	SEMICOLON,
	COLON,
	DOT,
	NEWLINE,
	EQUALS,
	OTHER
};

struct StringView {
	const char* ptr;
	size_t size;
};

struct Token {

	union{
		
		
		float float_literal;
		int int_literal;
	};
	StringView str;
	TokenType type;
};

#include<stdio.h>
#include<ctype.h>
bool is_letter(char character) {

	return isalpha(character);
}
bool is_number(char character) {
	return character >= '0' && character < '9';
}

const std::string keyword_list[] = {
	"struct",
	//"f32",
	//"i64",
	//"i32"
};

bool is_keyword(std::string_view str) {
	for (auto s : keyword_list) {
		if (s.compare(str) == 0) {
			return true;
		}
	}
	return false;
}
Token build_token(TokenType type, const char* start, const char* end) {
	Token tk;

	tk.type = type;

	size_t size = end - start;
	if (start == end)
	{
		size = 1;
	}

	tk.str.ptr = start;
	tk.str.size = size;

	std::string_view view{ start,size };
	if (type == TokenType::STRING) {
		if (is_keyword(view)) {
			tk.type = TokenType::KEYWORD;
			return tk;
		}

		//check for numerals
		bool hasF = view.back() == 'f';
		bool hasDot = false;

		if (hasF)
		{
			view = std::string_view{ start,size - 1 };
		}
		

		for (auto c : view) {
			if (c == '.') {
				hasDot = true;
			}
			else if (!is_number(c)) {
				//break all, not a number
				return tk;
			}
		}
		if (hasF && hasDot) {
			tk.type = TokenType::FLOAT_LITERAL;
			std::from_chars(start, start + size, tk.float_literal);
		}
		else {
			tk.type = TokenType::INT_LITERAL;

			std::from_chars(start, start + size, tk.int_literal);

		}
	}
	tk.str.ptr = start;
	tk.str.size = size;
	return tk;
}

CharacterType get_type(char character) {
	if (is_letter(character)) return CharacterType::LETTER;
	if (is_number(character)) return CharacterType::DIGIT;
	switch (character) {

	case '/': return CharacterType::SLASH; break;
	case '{': return CharacterType::CURLY_BRACKET_OPEN; break;
	case '}': return CharacterType::CURLY_BRACKET_CLOSE; break;
	case '[': return CharacterType::SQUARE_BRACKET_OPEN; break;
	case ']': return CharacterType::SQUARE_BRACKET_CLOSE; break;
	case ';': return CharacterType::SEMICOLON; break;
	case ':': return CharacterType::COLON; break;
	case ' ': return CharacterType::SPACE; break;
	case '.': return CharacterType::DOT; break;
	case '\n': return CharacterType::NEWLINE; break;
	case '=': return CharacterType::EQUALS; break;
	default: return CharacterType::OTHER;
	}
	return CharacterType::OTHER;
}

bool is_string_stop_character(char character) {
	CharacterType ctype = get_type(character);

	switch (ctype) {
	case CharacterType::SPACE:	
	case CharacterType::CURLY_BRACKET_OPEN:	
	case CharacterType::CURLY_BRACKET_CLOSE:	
	case CharacterType::SQUARE_BRACKET_OPEN:
	case CharacterType::SQUARE_BRACKET_CLOSE:
	case CharacterType::SLASH:	
	case CharacterType::SEMICOLON:	
	case CharacterType::COLON:	
	case CharacterType::NEWLINE:	
	case CharacterType::EQUALS:	
	case CharacterType::OTHER:
		return true;
	default:
		return false;

	}
}
//converts single-char to token, for the tokens that are 1 char
bool chartype_to_token(CharacterType ctype, TokenType& outType) {
	switch (ctype) {
	
	case CharacterType::CURLY_BRACKET_OPEN:
		outType = TokenType::CURLY_BRACKET_OPEN; return true;
	case CharacterType::CURLY_BRACKET_CLOSE:
		outType = TokenType::CURLY_BRACKET_CLOSE; return true;
	case CharacterType::SQUARE_BRACKET_OPEN:
		outType = TokenType::SQUARE_BRACKET_OPEN; return true;
	case CharacterType::SQUARE_BRACKET_CLOSE:
		outType = TokenType::SQUARE_BRACKET_CLOSE; return true;	
	case CharacterType::SEMICOLON:
		outType = TokenType::SEMICOLON; return true;
	case CharacterType::COLON:
		outType = TokenType::COLON; return true;	
	case CharacterType::EQUALS:
		outType = TokenType::EQUALS; return true;		
	default:
		return false;
	}
}

cppcoro::generator<Token> parse_string(const char* pars) {

	while (*pars) {
		CharacterType ctype = get_type(*pars);
		TokenType ttype = TokenType::NONE;
		if (chartype_to_token(ctype, ttype))
		{
			co_yield build_token(ttype, pars, pars);			
		}
		else
		{		
			switch (ctype)
			{			

			case CharacterType::DOT:
			case CharacterType::LETTER:
			case CharacterType::DIGIT:
				
				{
					//start peekforward
					const char* next = pars + 1;
					if (*next)
					{
						//loop until end-string letter
						while (*next && !is_string_stop_character(*next)) {
							next++;
						}

						co_yield build_token(TokenType::STRING, pars, next);

						pars = next;
						continue;
					}					
				}

				break;
			case CharacterType::SLASH:				
				
				//peek to see its second slash
				{
					const char* next = pars + 1;
					if (*next && get_type(*next) == CharacterType::SLASH)
					{
						next++;
						//loop until newline
						while (*next && get_type(*next) != CharacterType::NEWLINE) {
							next++;
						}
						
						co_yield build_token(TokenType::COMMENT, pars, next);
						pars = next;
					}
				}
				break;
			
			case CharacterType::OTHER:
				break;
			default:				
				break;
			} 
		}
		pars++;
	}

	co_return;
}
cppcoro::generator<Token> filter_comments(cppcoro::generator<Token>& tokens)
{
	for (auto tk : tokens) {
		if (tk.type != TokenType::COMMENT)
		{
			co_yield tk;
		}
	}
	co_return;
}

void print_token(const Token& token) {
	std::string_view stv{ token.str.ptr,token.str.size };

	if (token.type != TokenType::INT_LITERAL)
	{
		fmt::print("{} :: {} \n", to_string(token.type), stv);
	}
	else {
		fmt::print("{} :: i = {} \n", to_string(token.type), token.int_literal);
	}
	

}
