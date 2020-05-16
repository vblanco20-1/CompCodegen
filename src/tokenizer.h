#pragma once
#include <cppcoro/generator.hpp>
#include <iostream>
#include <charconv>
#include "tokenizer_types.h"


#include<stdio.h>
#include<ctype.h>


using TokenStream = cppcoro::generator<Token>;

inline bool is_letter(char character) {

	return isalpha(character);
}
inline bool is_number(char character) {
	return character >= '0' && character < '9';
}

const std::string keyword_list[] = {
	"struct",
	//"f32",
	//"i64",
	//"i32"
};

inline bool is_keyword(std::string_view str) {
	for (auto s : keyword_list) {
		if (s.compare(str) == 0) {
			return true;
		}
	}
	return false;
}
inline Token build_token(TokenType type, const char* start, const char* end) {
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
	if (type == TokenType::IDENTIFIER) {
		if (is_keyword(view)) {
			tk.type = TokenType::KEYWORD;
			return tk;
		}

		bool hasDot = false;
		for (auto c : view) {
			if (c == '.') {
				hasDot = true;
			}			
		}

		if (!hasDot) {
			auto intresult = std::from_chars(start, start + size, tk.int_literal);

			if (intresult.ec == std::errc())
			{
				tk.type = TokenType::INT_LITERAL;
			}
		}
		else {
			auto fltresult = std::from_chars(start, start + size, tk.float_literal);
			if (fltresult.ec == std::errc())
			{
				tk.type = TokenType::FLOAT_LITERAL;
			}
		}
	}
	tk.str.ptr = start;
	tk.str.size = size;
	return tk;
}

inline CharacterType get_type(char character) {
	if (is_letter(character)) return CharacterType::LETTER;
	if (is_number(character)) return CharacterType::DIGIT;
	switch (character) {
	case '_': return CharacterType::LETTER; break;
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
	case '#': return CharacterType::HASH; break;
	case ',': return CharacterType::COMMA; break;
	case '+': return CharacterType::PLUS; break;
	case '-': return CharacterType::MINUS; break;
	default: return CharacterType::OTHER;
	}
	return CharacterType::OTHER;
}

inline bool is_string_stop_character(char character) {
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
	case CharacterType::HASH:
	case CharacterType::COMMA:
	case CharacterType::OTHER:
		return true;
	default:
		return false;

	}
}
//converts single-char to token, for the tokens that are 1 char
//bool chartype_to_token(CharacterType ctype, TokenType& outType) {
//	switch (ctype) {
//	
//	case CharacterType::CURLY_BRACKET_OPEN:
//		outType = TokenType::CURLY_BRACKET_OPEN; return true;
//	case CharacterType::CURLY_BRACKET_CLOSE:
//		outType = TokenType::CURLY_BRACKET_CLOSE; return true;
//	case CharacterType::SQUARE_BRACKET_OPEN:
//		outType = TokenType::SQUARE_BRACKET_OPEN; return true;
//	case CharacterType::SQUARE_BRACKET_CLOSE:
//		outType = TokenType::SQUARE_BRACKET_CLOSE; return true;	
//	case CharacterType::SEMICOLON:
//		outType = TokenType::SEMICOLON; return true;
//	case CharacterType::COLON:
//		outType = TokenType::COLON; return true;	
//	case CharacterType::EQUALS:
//		outType = TokenType::EQUALS; return true;		
//	case CharacterType::COMMA:
//		outType = TokenType::COMMA; return true;
//	default:
//		return false;
//	}
//}

inline bool char_to_symbol(char c, SymbolType& outType) {

	for (auto sy : symbol_list) {
		if (*sy.symbol == c)
		{
			outType = sy.type;
			return true;
		}
	}
	return false;
}

inline bool check_token(span<Token> tokens, int index, TokenType type)
{
	return index < tokens.size() && tokens[index].type == type;
}

template<size_t Count>
bool check_token_chain(span<Token> tokens, int first, const std::array<TokenType, Count>& types)
{
	for (int i = 0; i < Count; i++)
	{
		if (!check_token(tokens, i + first, types[i]))
		{
			return false;
		}
	}
	return true;
}


inline bool check_token_symbol(const Token& tokens, SymbolType type)
{
	return tokens.type == TokenType::SYMBOL && tokens.symbol == type;
}
inline bool check_token_symbol(span<Token> tokens, int index, SymbolType type)
{
	return tokens[index].type == TokenType::SYMBOL && tokens[index].symbol == type;
}

template<size_t Count>
bool check_symbol_chain(span<Token> tokens, int first, const std::array<SymbolType, Count>& types)
{
	for (int i = 0; i < Count; i++)
	{
		if (types[i] != SymbolType::NONE &&  !check_token_symbol(tokens, i + first, types[i]))
		{
			return false;
		}
	}
	return true;
}

inline TokenStream parse_string(const char* pars) {

	bool bInsideReflectionBlock = false;
	while (*pars) {
		CharacterType ctype = get_type(*pars);
		SymbolType symbol;
		TokenType ttype = TokenType::NONE;
		if (char_to_symbol(*pars, symbol))
		{

			//minus and plus are special symbols, they might mark the start of a number
			if (symbol == SymbolType::MINUS || symbol == SymbolType::PLUS)
			{			
				
				//start peekforward
				const char* next = pars + 1;
				if (*next)
				{
					//loop until end-string letter
					while (*next && !is_string_stop_character(*next)) {
						next++;
					}

					co_yield build_token(TokenType::IDENTIFIER, pars, next);

					pars = next;
					continue;
				}				
			}

			Token symbolToken;
			symbolToken.symbol = symbol;
			symbolToken.str.ptr = pars;
			symbolToken.str.size = 1;
			symbolToken.type = TokenType::SYMBOL;

			co_yield symbolToken;

		}
		else
		{		
			switch (ctype)
			{
			
			//number
			case CharacterType::DIGIT:
			case CharacterType::MINUS:
			case CharacterType::PLUS:

			{
				//start peekforward
				const char* next = pars + 1;
				if (*next)
				{
					//loop until end-string letter
					while (*next && !is_string_stop_character(*next)) {
						next++;
					}

					co_yield build_token(TokenType::IDENTIFIER, pars, next);

					pars = next;
					continue;
				}
			}
			break;
			
			case CharacterType::LETTER:
		
				
				{
					//start peekforward
					const char* next = pars + 1;
					if (*next)
					{
						//loop until end-string letter
						while (*next && !is_string_stop_character(*next)) {
							next++;
						}

						co_yield build_token(TokenType::IDENTIFIER, pars, next);

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
						
						//co_yield build_token(TokenType::COMMENT, pars, next);
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
inline TokenStream filter_comments(TokenStream& tokens)
{
	for (auto tk : tokens) {
		if (tk.type != TokenType::COMMENT)
		{
			co_yield tk;
		}
	}
	co_return;
}

inline void print_token(const Token& token) {
	std::string_view stv{ token.str.ptr,token.str.size };

	switch (token.type)
	{
	case TokenType::INT_LITERAL:
		fmt::print("{} ::i = {} \n", to_string(token.type), stv, token.int_literal);
		break;
	case TokenType::SYMBOL:
		fmt::print("{} :: symbol {} = {} \n", to_string(token.type), stv, to_string(token.symbol));
		
		break;
	default:
		fmt::print("{} :: {} \n", to_string(token.type), stv);
		break;
	}
}

template<typename F>
void split_iterate(span<Token> tokens, SymbolType splitType, F&& fn) {
	int start = 0;
	int end = 0;

	for (int i = 0; i < tokens.size(); i++) {
		if (check_token_symbol(tokens, i, splitType)) {
			end = i;
			span<Token> segment;
			segment._begin = tokens._begin + start;
			segment._end = tokens._begin + end;
			fn(segment);
			start = i + 1;
		}
	}
	span<Token> segment;
	segment._begin = tokens._begin + start;
	segment._end = tokens._begin + tokens.size();
	fn(segment);
}


template<typename F>
void split_iterate(span<Token> tokens, TokenType splitType, F&& fn) {
	int start = 0;
	int end = 0;

	for (int i = 0; i < tokens.size(); i++) {
		if (tokens[i].type == splitType) {
			end = i;
			span<Token> segment;
			segment._begin = tokens._begin + start;
			segment._end = tokens._begin + end;
			fn(segment);
			start = i + 1;
		}
	}
	span<Token> segment;
	segment._begin = tokens._begin + start;
	segment._end = tokens._begin + tokens.size();
	fn(segment);
}