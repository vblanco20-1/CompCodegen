// CompCodegen.cpp : Defines the entry point for the application.
//

#include "CompCodegen.h"


#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include <fstream> 
#include <string>
#include <vector>
#include <iostream>

#include "tokenizer.h"
#include <sstream>
using json = nlohmann::json;
using namespace fmt::literals;

enum class MetadataType : char{
	FLOAT,
	INT,
	STRING,
	NONE
};

struct Metadata {
	union {
		float float_md;
		int int_md;
	};
	MetadataType type;
};
struct Parameter
{
	std::unordered_map<std::string, Metadata> metadata;
	std::string name;
	std::string type_name;
	int array_lenght = -1;
};

struct Component
{
	std::vector<Parameter> parameters;
	std::string name;
};

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

//<parameter> = <name> <:> <type> <array> <default> <comma>
//<default> = nothing
//<default> = <equals> <literal>
//<array> = nothing
//<array> = <open sqbracket> <int> <close sqbracket>
bool parse_parameter(span<Token> tokens, Parameter& outC) {

	if(tokens.back().type != TokenType::SEMICOLON)return false;

	Token name = tokens[0];

	if (name.type != TokenType::STRING) return false;

	Token colon = tokens[1];

	if (colon.type != TokenType::COLON) return false;

	Token ptype = tokens[2];

	if (ptype.type != TokenType::STRING) return false;

	//array handling
	if (tokens[3].type == TokenType::SQUARE_BRACKET_OPEN)
	{
		//unsized array
		if (tokens[4].type == TokenType::SQUARE_BRACKET_CLOSE)
		{
			outC.array_lenght = 0;
		}
		//array size
		else if (tokens[4].type == TokenType::INT_LITERAL &&  (tokens[5].type == TokenType::SQUARE_BRACKET_CLOSE))
		{
			outC.array_lenght = tokens[4].int_literal;
		}
		else
		{
			return false;
		}
	}

	//find metadata zone
	int mt_start = -1;
	int mt_end = -1;
	for (int i = 0; i < tokens.size(); i++) {
		if (tokens[i].type == TokenType::REFLINFO_BEGIN)
		{
			mt_start = i;
		}
		else if (tokens[i].type == TokenType::REFLINFO_END)
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
		split_iterate(segment, TokenType::COMMA, [&metadata](span<Token> tks) {
			if (tks[0].type == TokenType::STRING)
			{
				if (tks.size() == 1) {
					Metadata mt;
					mt.type = MetadataType::NONE;
					metadata[std::string(tks[0].view())] = mt;
				}
				else if (tks.size() == 3 && tks[1].type == TokenType::EQUALS) {
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
	outC.name = std::string_view{ name.str.ptr,name.str.size };
	outC.type_name = std::string_view{ ptype.str.ptr,ptype.str.size };

	return true;
}

//<component> = <struct> <name> <open_bracket> <inner> <close_bracket>
//<inner> = array<parameter>

bool parse_component(span<Token> tokens, Component& outC) {
	
	Component newComp;

	if (tokens[0].type != TokenType::KEYWORD) return false;

	Token compname = tokens[1];

	if (compname.type != TokenType::STRING) return false;

	if (tokens[2].type != TokenType::CURLY_BRACKET_OPEN) return false;

	newComp.name = std::string_view{ compname.str.ptr,compname.str.size };

	//cut the parameter token streams
	int _cursor = 3;
	while (tokens[_cursor].type != TokenType::CURLY_BRACKET_CLOSE) {
		int _cursorend = -1;
		//find next comma
		for (int i = _cursor; i < tokens.size(); i++) {

			//end of comp
			if (tokens[i].type == TokenType::SEMICOLON)
			{
				_cursorend = i;
				break;
			}
		}
		if (_cursorend > _cursor) {
			Parameter param;

			span<Token> paramTokens;
			paramTokens._begin = &tokens[_cursor];
			paramTokens._end = &tokens[_cursorend] +1;
			if (parse_parameter(paramTokens, param)) {
				newComp.parameters.push_back(param);
				_cursor = _cursorend+1;
			}
			else // error when parsing param
			{
				return false;
			}
		}
	}
	outC = newComp;
	return true;
}

struct DocLine
{
	std::string line;
	int indent;
};
struct Document
{
	std::vector<DocLine> lines;

	int current_indent{ 0 };

	void add_line(std::string line)
	{
		lines.push_back({ line,current_indent });
	}
};


Component parse_component(std::string_view name, const json& in_object)
{
	Component cmp;

	cmp.name = name;

	for (auto& [name, obj] : in_object["variables"].items())
	{
		Parameter newParam;

		newParam.name = name;
		newParam.type_name = obj;
		cmp.parameters.push_back(newParam);
	}

	return cmp;
};

void write_parameter(const  Parameter& param, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//newval -------");
	outdc.add_line(fmt::format("{} {};",param.type_name, param.name));
};

std::string quoted(const std::string& original) {
	return fmt::format("\"{}\"",original);
}

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
	outdc.add_line(fmt::format("struct {} {",cmp.name));
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

void write_imgui_param(const Parameter& param, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//edit -------");

	std::string name_quoted = quoted(param.name);
	if (param.type_name.compare("f32") == 0)
	{	
		outdc.add_line(fmt::format("ImGui::InputFloat({},&component.{});", name_quoted, param.name));
	}
	else if (param.type_name.compare("vec3") == 0)
	{
		outdc.add_line(fmt::format("ImGui::InputFloat3({},&component.{}[0]);", name_quoted, param.name));
	}
	else
	{
		outdc.add_line(fmt::format("// parameter {} not editable", param.name));
	}

};

void write_imgui_edit(const Component& cmp, Document& outdc)
{
	outdc.add_line("//editor");
	outdc.add_line(fmt::format("void imgui_edit_comp_{type}(const {type} &component) {{", "type"_a = cmp.name));
	outdc.current_indent++;

	for (auto& p : cmp.parameters)
	{
		write_imgui_param(p, outdc);
	}

	outdc.current_indent--;
	outdc.add_line("}");
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

int main(int argc, char* argv[])
{
	std::vector<Component> component_table;

	//std::ifstream i("test_files/test_components.json");
	std::ifstream i("test_files/basic_struct.txt");
	if (i.is_open()) {

		std::string str((std::istreambuf_iterator<char>(i)),
			std::istreambuf_iterator<char>());

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

		Component comp;
		parse_component(cmptokens, comp);

		component_table.push_back(comp);
#if 0
		json j;
		i >> j;

		auto complist = j["component_definitions"];
		for (auto& [name, obj] : complist.items())
		{
			Component cmp = parse_component(name, complist[name]);
			component_table.push_back(cmp);


		}

		
#endif

		Document doc;

		for (auto& c : component_table)
		{
			//write_component(c,doc);
			write_component_unreal(c, doc);
		}

		for (auto& c : component_table)
		{
			//write_component(c,doc);
			write_imgui_edit(c, doc);
		}

		write_entt_edit(component_table, doc);

		output_document(std::cout, doc);
	}
	

	return 0;
}
