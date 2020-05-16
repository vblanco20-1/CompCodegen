#pragma once
#include "document_output.h"
#include "parser_types.h"
#include "parser.h"

#include "fmt/format.h"
#include <fstream> 
#include <string>
#include <vector>
#include <sstream>

void write_struct_parameter(const  Parameter& param, Document& outdc)
{
	outdc.add_line("");

	if (param.array_lenght == -1)
	{
		outdc.add_line(fmt::format("{} {};", param.type->real_name, param.name));
	}
	else if (param.array_lenght == 0)
	{
		outdc.add_line(fmt::format("std::vector<{}> {};", param.type->real_name, param.name));
	}
	else //sized array
	{
		outdc.add_line(fmt::format("{} {}[{}];", param.type->real_name, param.name, param.array_lenght));
	}


};

void write_struct(StructDefinition* stc, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//component-------");
	outdc.add_line(fmt::format("struct {} {{", stc->real_name));
	outdc.current_indent++;
	for (auto& p : stc->parameters)
	{
		write_struct_parameter(p, outdc);
	}
	outdc.current_indent--;
	outdc.add_line("};");
};
std::string quoted(const std::string& original) {
	return fmt::format("\"{}\"", original);
}
void write_imgui_param(Parameter& param, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line("//edit -------");

	std::string name_quoted = quoted(param.name);
	if (param.type_name.compare("f32") == 0)
	{
		if (has_metadata("slider_min", param) && has_metadata("slider_max", param))
		{
			float slider_min = param.metadata["slider_min"].float_md;
			float slider_max = param.metadata["slider_max"].float_md;
			outdc.add_line(fmt::format("ImGui::SliderFloat({},&component.{},{},{});", name_quoted, param.name, slider_min, slider_max));
		}
		else
		{
			outdc.add_line(fmt::format("ImGui::InputFloat({},&component.{});", name_quoted, param.name));
		}

	}
	else if (param.type_name.compare("i32") == 0)
	{

		outdc.add_line(fmt::format("ImGui::InputInt({},&component.{});", name_quoted, param.name));
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
std::string get_imgui_edit_name(StructDefinition* stc) {
	return fmt::format("void imgui_edit_comp_{}(const {} &component)",  stc->real_name, stc->real_name);
}

void write_imgui_edit(StructDefinition* stc, Document& outdc)
{
	outdc.add_line("//editor");
	outdc.add_line(fmt::format("{} {{", get_imgui_edit_name(stc)));
	outdc.current_indent++;

	for (auto& p : stc->parameters)
	{
		write_imgui_param(p, outdc);
	}

	outdc.current_indent--;
	outdc.add_line("}");
}

void fwd_declare_struct(StructDefinition* stc, Document& outdc)
{
	outdc.add_line("");	
	outdc.add_line(fmt::format("struct {} ;", stc->real_name));
}



bool fwd_declare_edit_function(StructDefinition* stc, Document& outdc)
{
	outdc.add_line("");
	outdc.add_line(fmt::format("{} ;", get_imgui_edit_name(stc)));
	return true;
}

bool output_module(struct Module* mod, Document& outHeader, Document& outSource, const OutputOptions& options)
{

	outHeader.add_line("#pragma once");
	outHeader.add_line("#include <vector>");

	outSource.add_line(fmt::format("#include {}", outHeader.name));
	outSource.add_line("#include \"IMGUI/imgui.h\"");


	//forward declaration
	for (auto [k,v] : mod->nodes)
	{
		if (v->type == RootType::STRUCT)
		{
			StructDefinition* stc = static_cast<StructDefinition*>(v);
			if (!stc->bIsDefault)
			{
				fwd_declare_struct(stc, outHeader);
				fwd_declare_edit_function(stc, outHeader);
			}
		}
	}

	//definitions
	for (auto [k, v] : mod->nodes)
	{
		if (v->type == RootType::STRUCT)
		{
			StructDefinition* stc = static_cast<StructDefinition*>(v);
			if (!stc->bIsDefault)
			{
				write_struct(stc, outHeader);
				write_imgui_edit(stc, outSource);
			}
		}
	}



	return true;
}
