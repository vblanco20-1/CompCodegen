#pragma once
#include "parser_types.h"


bool add_struct_definition(std::string_view struct_name,std::vector<Token> && tokens, Module* mod);

bool parse_stream(cppcoro::generator<Token>& token_stream, Module* mod);

bool has_metadata(std::string metadata, const Parameter& param);
