#pragma once
#include <string_view>

template<typename T>
struct span {
	T* _end;
	T* _begin;

	T& operator[](size_t idx)
	{
		return _begin[idx];
	}

	T& back() {
		return *(_end - 1);
	}

	size_t size() {
		return _end - _begin;
	}
};
enum class TokenType : char {
	IDENTIFIER,
	KEYWORD,

	COMMENT,

	FLOAT_LITERAL,
	INT_LITERAL,
	STRING_LITERAL,
	SYMBOL,
	NONE
};

enum class SymbolType :char {
	EQUALS,
	SEMICOLON,
	COMMA,
	COLON,
	CURLY_BRACKET_OPEN,
	CURLY_BRACKET_CLOSE,
	SQUARE_BRACKET_OPEN,
	SQUARE_BRACKET_CLOSE,
	MINUS,
	PLUS,
	DOT,
	HASH,
	NONE
};

struct SymbolDefinition {
	SymbolType type;
	const char* symbol;
};

const SymbolDefinition symbol_list[]{
	{SymbolType::EQUALS,"="},
	{SymbolType::SEMICOLON,";"},
	{SymbolType::COMMA,","},
	{SymbolType::COLON,":"},
	{SymbolType::CURLY_BRACKET_OPEN,"{"},
	{SymbolType::CURLY_BRACKET_CLOSE,"}"},
	{SymbolType::SQUARE_BRACKET_OPEN,"["},
	{SymbolType::SQUARE_BRACKET_CLOSE,"]"},
	{SymbolType::HASH,"#"},
	{SymbolType::MINUS,"-"},
	{SymbolType::PLUS,"+"},
	{SymbolType::DOT,"."}
};

inline std::string to_string(SymbolType ttype)
{
	switch (ttype)
	{
	case SymbolType::SEMICOLON:
		return "SEMICOLON";
		break;
	case SymbolType::EQUALS:
		return "EQUALS";
		break;
	case SymbolType::COLON:
		return "COLON";
		break;
	case SymbolType::CURLY_BRACKET_OPEN:
		return "CURLY BRACKET OPEN";
		break;
	case SymbolType::CURLY_BRACKET_CLOSE:
		return "CURLY BRACKET CLOSE";
		break;
	case SymbolType::SQUARE_BRACKET_OPEN:
		return "SQUARE BRACKET OPEN";
		break;
	case SymbolType::SQUARE_BRACKET_CLOSE:
		return "SQUARE BRACKET CLOSE";
		break;
	case SymbolType::MINUS:
		return "MINUS";
		break;
	case SymbolType::PLUS:
		return "PLUS";
		break;
	case SymbolType::DOT:
		return "DOT";
		break;
	case SymbolType::HASH:
		return "HASH";
		break;
	case SymbolType::COMMA:
		return "COMMA";
		break;

	default:
		return "ERROR";
		break;
	}
}
inline std::string to_string(TokenType ttype) {
	switch (ttype)
	{
	case TokenType::IDENTIFIER:
		return "STRING";
		break;
	case TokenType::KEYWORD:
		return "KEYWORD";
		break;

	case TokenType::COMMENT:
		return "COMMENT";
		break;

	case TokenType::FLOAT_LITERAL:
		return "FLOAT";
		break;
	case TokenType::INT_LITERAL:
		return "INT";
		break;
	case TokenType::SYMBOL:
		return "SYMBOL";
		break;
	case TokenType::STRING_LITERAL:
		return "STRING LITERAL";
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
	HASH,
	COMMA,
	PLUS,
	MINUS,
	OTHER
};

struct StringView {
	const char* ptr;
	size_t size;
};

struct Token {

	union {

		SymbolType symbol;
		float float_literal;
		int int_literal;
	};
	StringView str;
	TokenType type;

	std::string_view view() {
		return std::string_view{ str.ptr,str.size };
	}
};