// CompCodegen.cpp : Defines the entry point for the application.
//

#include "CompCodegen.h"

#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
using json = nlohmann::json;
using namespace fmt::literals;
struct Parameter
{
	std::string name;
	std::string type_name;
};

struct Component
{
	std::vector<Parameter> parameters;
	std::string name;
};

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
	outdc.add_line(fmt::format("{} {};", to_unreal_type(param.type_name), param.name));
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

	std::ifstream i("test_components.json");
	if (i.is_open()) {
		json j;
		i >> j;

		auto complist = j["component_definitions"];
		for (auto& [name, obj] : complist.items())
		{
			Component cmp = parse_component(name, complist[name]);
			component_table.push_back(cmp);


		}

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
