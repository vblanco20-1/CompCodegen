#pragma once
#include <string>
#include <vector>

struct DocLine
{
	std::string line;
	int indent;
};
struct Document
{
	std::string name;
	std::vector<DocLine> lines;

	int current_indent{ 0 };

	void add_line(std::string line)
	{
		lines.push_back({ line,current_indent });
	}
};

struct OutputOptions
{

};
bool output_module(struct Module* mod, Document& outHeader, Document& outSource,const OutputOptions &options);