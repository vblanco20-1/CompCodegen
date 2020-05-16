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
	}

	return 0;
}
