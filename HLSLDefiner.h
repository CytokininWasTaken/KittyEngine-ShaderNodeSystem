#pragma once
#include "LanguageData.h"

namespace KE
{
	class HLSLDefiner
	{
	public:
		enum class Annotations
		{
			Entrypoint,
			Function,
			Buffer,
			Struct,
			Texture2D,
			Intrinsic,
			Operator,
		};

		struct LangDefinition : public LanguageDefinitionNew
		{
			std::unordered_map<std::string, StructDefinition> structs;
			std::unordered_map<std::string, BufferDefinition> buffers;
			std::unordered_map<std::string, FunctionDefinition> functions;
			std::unordered_map<std::string, DataType> dataTypes;
			std::unordered_map<std::string, EntrypointDefinition> entrypoints;
			std::unordered_map<std::string, TextureDefinition> textures;
		};

	private:
	
	public:
		static void Interpret(LangDefinition& aDefinition, const AnnotatedCode<Annotations>& aCode);
		static void RegisterAnnotations(std::unordered_map<std::string, Annotations>& anAnnotationMap);


	};

}