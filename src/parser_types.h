#pragma once
#include <vector>
#include <unordered_map>
#include "tokenizer_types.h"

struct Module {
	std::unordered_map<std::string,struct RootNode*> nodes;
	std::unordered_map<std::string,struct RootNode*> aliases;

	void initialize_default_types();
};


enum class RootType {
	STRUCT
};

struct RootNode {
	std::string real_name;
	RootType type;
	std::vector<Token> tokens;
	Module* owner;
};

enum class MetadataType : char {
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
	struct StructDefinition* type;
	int array_lenght = -1;
};

struct StructDefinition : public RootNode {
	StructDefinition():bIsDefault(false) { type = RootType::STRUCT; };	
	std::vector<Parameter> parameters;
	bool bIsDefault;
};