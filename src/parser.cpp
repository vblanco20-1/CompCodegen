#pragma once
#include "parser.h"
#include "tokenizer.h"

bool parse_struct_parameter(span<Token> tokens, Parameter& outC);
bool parse_struct(StructDefinition* definition, Module* mod);


StructDefinition* allocateStruct(Module* mod, std::string_view real_name, bool b_default_type = false) {
	StructDefinition* new_struct = new StructDefinition();
	new_struct->real_name = real_name;
	new_struct->owner = mod;
	new_struct->bIsDefault = b_default_type;
	mod->nodes[new_struct->real_name] = new_struct;
	return new_struct;
}

void alias_type(Module* mod, std::string_view real_name, std::string_view alias) {

	mod->aliases[std::string(alias)] = mod->nodes[std::string(real_name)];
}
void Module::initialize_default_types()
{
	StructDefinition* s_float = allocateStruct(this, "float",true);
	StructDefinition* s_int = allocateStruct(this, "int", true);
	StructDefinition* s_int64 = allocateStruct(this, "long int", true);
	StructDefinition* s_double = allocateStruct(this, "double", true);
	StructDefinition* s_uint = allocateStruct(this, "unsigned int", true);

	alias_type(this, "float","f32");
	alias_type(this, "double", "f64");

	alias_type(this, s_int->real_name, "i32");
	alias_type(this, s_uint->real_name, "u32");
	alias_type(this, s_int64->real_name, "i64");
	
}


bool add_struct_definition(std::string_view struct_name, std::vector<Token>&& tokens, Module* mod)
{
	StructDefinition* new_struct = allocateStruct(mod,struct_name);
	new_struct->tokens = std::move(tokens);

	return parse_struct(new_struct,mod);
}

StructDefinition* find_type(std::string struct_name, Module* mod)
{
	auto it = mod->nodes.find(struct_name);
	if (it != mod->nodes.end()) {
		return static_cast<StructDefinition*>( (it)->second);
	}
	else
	{
		auto it2 = mod->aliases.find(struct_name);
		if (it2 != mod->aliases.end()) {
			return static_cast<StructDefinition*>((it2)->second);
		}
	}
	return nullptr;
}
//<parameter> = <name> <:> <type> <array> <default> <comma>
//<default> = nothing
//<default> = <equals> <literal>
//<array> = nothing
//<array> = <open sqbracket> <int> <close sqbracket>
bool parse_struct_parameter(span<Token> tokens, Parameter& outC) {

	if (!check_token_symbol(tokens, tokens.size() - 1, SymbolType::SEMICOLON))return false;


	if (!check_token_chain<3>(tokens, 0, { TokenType::IDENTIFIER ,TokenType::SYMBOL ,TokenType::IDENTIFIER })) return false;


	if (!check_token_symbol(tokens, 1, SymbolType::COLON)) return false;

	//array parsing
	if (check_token_symbol(tokens, 3, SymbolType::SQUARE_BRACKET_OPEN))
	{
		bool bUnsizedArray = check_token_symbol(tokens, 3, SymbolType::SQUARE_BRACKET_OPEN) && check_token_symbol(tokens, 4, SymbolType::SQUARE_BRACKET_CLOSE);

		bool bSizedArray = check_token_chain<3>(tokens, 3, { TokenType::SYMBOL ,TokenType::INT_LITERAL ,TokenType::SYMBOL });

		bSizedArray = bSizedArray && check_token_symbol(tokens, 3, SymbolType::SQUARE_BRACKET_OPEN) && check_token_symbol(tokens, 5, SymbolType::SQUARE_BRACKET_CLOSE);

		if (bUnsizedArray)
		{
			outC.array_lenght = 0;
		}
		else if (bSizedArray) {
			outC.array_lenght = tokens[4].int_literal;
		}
		else {
			return false;
		}
	}

	//find metadata zone
	int mt_start = -1;
	int mt_end = -1;
	for (int i = 0; i < tokens.size(); i++) {
		if (check_symbol_chain<2>(tokens, i, { SymbolType::HASH ,SymbolType::SQUARE_BRACKET_OPEN }))
		{
			mt_start = i + 1;
		}
		else if (check_token_symbol(tokens, i, SymbolType::SQUARE_BRACKET_CLOSE))
		{
			mt_end = i;
		}
	}
	//reflection zone found, parse it
	if (mt_start > 0 && mt_end > mt_start) {
		span<Token> segment;
		segment._begin = tokens._begin + mt_start + 1;
		segment._end = tokens._begin + mt_end;

		std::unordered_map<std::string, Metadata> metadata;

		//2 options, either <string> <equals> <literal> or <string>
		split_iterate(segment, SymbolType::COMMA, [&metadata](span<Token> tks) {
			if (tks[0].type == TokenType::IDENTIFIER)
			{
				if (tks.size() == 1) {
					Metadata mt;
					mt.type = MetadataType::NONE;
					metadata[std::string(tks[0].view())] = mt;
				}
				else if (tks.size() == 3 && check_token_symbol(tks, 1, SymbolType::EQUALS)) {
					Metadata mt;


					if (tks[2].type == TokenType::INT_LITERAL)
					{
						mt.type = MetadataType::INT;
						mt.int_md = tks[2].int_literal;
					}
					else if (tks[2].type == TokenType::FLOAT_LITERAL)
					{
						mt.type = MetadataType::FLOAT;
						mt.float_md = tks[2].float_literal;
					}

					metadata[std::string(tks[0].view())] = mt;
				}
			}

			});

		outC.metadata = std::move(metadata);
	}


	//no literal handling yet
	outC.name = std::string_view{ tokens[0].str.ptr,tokens[0].str.size };
	outC.type_name = std::string_view{ tokens[2].str.ptr,tokens[2].str.size };
	
	return true;
}

bool parse_struct(StructDefinition* definition, Module* mod) {
	
	span<Token> tokens;
	tokens._begin = definition->tokens.data();
	tokens._end = definition->tokens.data() + definition->tokens.size();

	if (!check_token(tokens, 0, TokenType::KEYWORD)) return false;

	Token compname = tokens[1];

	if (!check_token(tokens, 1, TokenType::IDENTIFIER)) return false;

	if (!check_token_symbol(tokens, 2, SymbolType::CURLY_BRACKET_OPEN)) return false;	

	//an empty component has exactly 4 tokens
	if (tokens.size() > 4) {

		//cut the parameter token streams
		int _cursor = 3;
		while (true) {
			int _cursorend = -1;
			//find next comma
			for (int i = _cursor; i < tokens.size(); i++) {

				//end of comp
				if (check_token_symbol(tokens, i, SymbolType::SEMICOLON))
				{
					_cursorend = i;
					break;
				}
			}
			if (_cursorend > _cursor) {
				Parameter param;

				span<Token> paramTokens;
				paramTokens._begin = &tokens[_cursor];
				paramTokens._end = &tokens[_cursorend] + 1;
				if (parse_struct_parameter(paramTokens, param)) {
					param.type = find_type(param.type_name, mod);
					if (param.type == nullptr)
					{
						return false;
					}
					definition->parameters.push_back(param);
					_cursor = _cursorend + 1;
				}
				else // error when parsing param
				{
					return false;
				}
			}
			else
			{
				break;
			}
		}
	}	

	return true;
}

bool parse_stream(cppcoro::generator<Token>& token_stream, Module* mod)
{
	Token tk;
	auto IT = token_stream.begin();
	auto end = token_stream.end();
	while (IT != end)
	{
		tk = *IT;
		if (tk.type == TokenType::KEYWORD) {
			std::vector<Token> struct_tokens;
			struct_tokens.push_back(tk);
			while (++IT != end)
			{
				tk = *(IT);
				struct_tokens.push_back(tk);
				if (check_token_symbol(tk, SymbolType::CURLY_BRACKET_CLOSE))
				{
					add_struct_definition(struct_tokens[1].view(), std::move(struct_tokens), mod);
					
					break;
				}
			}
			IT++;
		}
		else {
			return false;
		}
	}

	return false;
}

bool has_metadata(std::string metadata, const Parameter& param)
{
	auto it = param.metadata.find(metadata);
	return it != param.metadata.end();
}
