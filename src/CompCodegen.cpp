// CompCodegen.cpp : Defines the entry point for the application.
//

#include "CompCodegen.h"


//#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include <fstream> 
#include <string>
#include <vector>
#include <iostream>

#include "tokenizer.h"
#include <sstream>
#include "parser_types.h"
#include "parser.h"

#include "document_output.h"
//using json = nlohmann::json;
using namespace fmt::literals;




struct Component
{
	std::vector<Parameter> parameters;
	std::string name;
};




//<parameter> = <name> <:> <type> <array> <default> <comma>
//<default> = nothing
//<default> = <equals> <literal>
//<array> = nothing
//<array> = <open sqbracket> <int> <close sqbracket>
bool parse_parameter(span<Token> tokens, Parameter& outC) {
#if 1
	if(!check_token_symbol(tokens,tokens.size()-1,SymbolType::SEMICOLON))return false;
	

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
		if (check_symbol_chain<2>(tokens, i, { SymbolType::HASH ,SymbolType::SQUARE_BRACKET_OPEN }) )
		{
			mt_start = i+1;
		}
		else if (check_token_symbol(tokens, i, SymbolType::SQUARE_BRACKET_CLOSE))
		{
			mt_end = i;
		}
	}
	//reflection zone found, parse it
	if (mt_start > 0 && mt_end > mt_start) {
		span<Token> segment;
		segment._begin = tokens._begin + mt_start +1;
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
#endif
	return true;
}

//<component> = <struct> <name> <open_bracket> <inner> <close_bracket>
//<inner> = array<parameter>

bool parse_component(span<Token> tokens, Component& outC) {

	Component newComp;

	if (!check_token(tokens, 0, TokenType::KEYWORD) ) return false;

	Token compname = tokens[1];

	if (!check_token(tokens, 1, TokenType::IDENTIFIER)) return false;

	if (!check_token_symbol(tokens, 2, SymbolType::CURLY_BRACKET_OPEN)) return false;

	newComp.name = std::string_view{ compname.str.ptr,compname.str.size };

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
				if (parse_parameter(paramTokens, param)) {
					newComp.parameters.push_back(param);
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
	outC = newComp;

	return true;
}



//Component parse_component(std::string_view name, const json& in_object)
//{
//	Component cmp;
//
//	cmp.name = name;
//
//	for (auto& [name, obj] : in_object["variables"].items())
//	{
//		Parameter newParam;
//
//		newParam.name = name;
//		newParam.type_name = obj;
//		cmp.parameters.push_back(newParam);
//	}
//
//	return cmp;
//};

void write_parameter(const  Parameter& param, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//newval -------");
	outdc.add_line(fmt::format("{} {};",param.type_name, param.name));
};


std::string to_unreal_type(const std::string& type_name)
{
	if (type_name.compare("f32") == 0)
	{
		return "float";
	}
	else if (type_name.compare("i32") == 0)
	{
		return "int32_t";
	}
	else if (type_name.compare("i64") == 0)
	{
		return "int64_t";
	}
	else if (type_name.compare("vec3") == 0)
	{
		return "FVector";
	}

	return type_name;
}

void write_parameter_unreal(const  Parameter& param, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//newval -------");
	outdc.add_line("UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Component)");

	std::stringstream metadata_stream;
	metadata_stream << "//";
	bool hasMetadata = false;
	for (auto [k, v] : param.metadata) {
		hasMetadata = true;

		switch (v.type)
		{
		case MetadataType::FLOAT:
			metadata_stream << fmt::format("{} :float {} ,", k, v.float_md);
			break;
		case MetadataType::INT:
			metadata_stream << fmt::format("{} :int {} ,", k, v.int_md);
			break;
		case MetadataType::STRING:
			metadata_stream << fmt::format("{} :str  ,", k);
			break;
		case MetadataType::NONE:
			metadata_stream << fmt::format("{} ,", k);
			break;
		default:
			assert(false);
		}	
	}

	if (param.array_lenght == -1)
	{
		outdc.add_line(fmt::format("{} {};", to_unreal_type(param.type_name), param.name));
	}
	else if (param.array_lenght == 0)
	{
		outdc.add_line(fmt::format("TArray<{}> {};", to_unreal_type(param.type_name), param.name));
	}
	else //sized array
	{
		outdc.add_line(fmt::format("{} {}[{}];", to_unreal_type(param.type_name), param.name,param.array_lenght));
	}

	if (hasMetadata) {
		outdc.add_line( metadata_stream.str());
	}
	
};

void write_component(const Component& cmp, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//component-------");
	outdc.add_line(fmt::format("struct {} {{",cmp.name));
	outdc.current_indent++;
	for (auto& p : cmp.parameters)
	{
		write_parameter(p, outdc);
	}
	outdc.current_indent--;
	outdc.add_line("}");
};

void write_component_unreal(const Component& cmp, Document& outdc)
{

	std::string unrealname = "F" + cmp.name;

	outdc.add_line("");
	outdc.add_line("//component-------");
	outdc.add_line("USTRUCT(BlueprintType)");


	outdc.add_line(fmt::format("struct {} {{", unrealname));
	outdc.current_indent++;
	outdc.add_line("GENERATED_USTRUCT_BODY()");
	for (auto& p : cmp.parameters)
	{
		write_parameter_unreal(p, outdc);
	}
	outdc.current_indent--;
	outdc.add_line("}");
};


void write_imgui_edit( Component& cmp, Document& outdc)
{
	//outdc.add_line("//editor");
	//outdc.add_line(fmt::format("void imgui_edit_comp_{type}(const {type} &component) {{", "type"_a = cmp.name));
	//outdc.current_indent++;
	//
	//for (auto& p : cmp.parameters)
	//{
	//	write_imgui_param(p, outdc);
	//}
	//
	//outdc.current_indent--;
	//outdc.add_line("}");
}

void write_entt_edit(std::vector<Component>& components, Document& outdc)
{
	outdc.add_line("//editor entt");
	outdc.add_line("void edit_entity(entt::registry& rg, entt::entity id) {");
	outdc.current_indent++;

	for (auto& cmp : components)
	{

		outdc.add_line(fmt::format("if(rg.has<{type}>(id) {{", "type"_a = cmp.name));
		
		outdc.current_indent++;

		outdc.add_line(fmt::format("imgui_edit_comp_{type}(rg.get<{type}>(id));", "type"_a = cmp.name));


		outdc.current_indent--;
		outdc.add_line("}");
	}

	outdc.current_indent--;
	outdc.add_line("}");
}
template<typename Stream>
void output_document(Stream& stream, Document& doc)
{
	for (auto& line : doc.lines)
	{
		for (int i = 0; i < line.indent; i++)
		{
			stream << "\t";
		}
		stream << line.line << "\n";
	}
};

template<typename F>
void split_structs(span<Token> tokens, F&& fn) {
	int start = 0;
	int end = 0;

	for (int i = 0; i < tokens.size()-2; i++) {

		//find "struct" declaration, <struct> <name> <{>
		//if (check_token(tokens,i,TokenType::KEYWORD) && tokens[i+1].type == TokenType::IDENTIFIER && tokens[i + 2].type == TokenType::CURLY_BRACKET_OPEN)
		if (check_token(tokens, i, TokenType::KEYWORD) && check_token(tokens, i+1, TokenType::IDENTIFIER))
		{
			if (check_token_symbol(tokens, i + 2, SymbolType::CURLY_BRACKET_OPEN))
			{
				//find the closing brace
				for (int j = i + 3; j < tokens.size(); j++)
				{
					if (check_token_symbol(tokens, j, SymbolType::CURLY_BRACKET_CLOSE))
					{
						span<Token> segment;
						segment._begin = tokens._begin + i;
						segment._end = tokens._begin + j+1;
						fn(segment);
						break;
					}
				}
			}			
		}
	}

}

int main(int argc, char* argv[])
{
	std::vector<Component> component_table;

	//std::ifstream i("test_files/test_components.json");
	std::ifstream i("test_files/basic_struct.txt");
	if (i.is_open()) {

		std::string str((std::istreambuf_iterator<char>(i)),
			std::istreambuf_iterator<char>());

		Module schema;
		schema.initialize_default_types();
		{
			auto tokens = parse_string(str.c_str());
			auto tokens2 = filter_comments(tokens);

			parse_stream(tokens2, &schema);

			Document header;
			Document source;

			header.name = "comps.generated.h";
			source.name = "comps.generated.cpp";

			OutputOptions outputOptions;
			output_module(&schema, header, source, outputOptions);

			std::ofstream fheader("test_files/" + header.name);
			std::ofstream fsource("test_files/" + source.name);

			output_document(fheader, header);
			output_document(fsource, source);
		}

#if 0

		auto tokens = parse_string(str.c_str());
		auto tokens2 = filter_comments(tokens);


		std::vector<Token> copied_tokens;
		for (auto tk : tokens2) {
			print_token(tk);
			copied_tokens.push_back(tk);
		}

		span<Token> cmptokens;
		cmptokens._begin = copied_tokens.data();
		cmptokens._end = copied_tokens.data() + copied_tokens.size();

		split_structs(cmptokens, [&](span<Token> structtokens) {
			Component comp;
			if (parse_component(structtokens, comp))
			{
				component_table.push_back(comp);
			}			
		});

				Document doc;

		for (auto& c : component_table)
		{
			write_component(c,doc);
			//write_component_unreal(c, doc);
		}

		for (auto& c : component_table)
		{
			//write_component(c,doc);
			write_imgui_edit(c, doc);
		}

		write_entt_edit(component_table, doc);

		output_document(std::cout, doc);

#endif
	}

	return 0;
}
