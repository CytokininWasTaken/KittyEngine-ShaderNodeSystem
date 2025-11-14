#pragma once
#include <functional>

#include "nlohmann/json.hpp"

#define DefinerMember typename DefinerClass

namespace KE
{
	inline std::string GetFileContents(const char* aFilePath)
	{
		std::ifstream file(aFilePath);
		if (!file.is_open()) { return ""; }

		std::stringstream buffer;
		buffer << file.rdbuf();

		return buffer.str();
	}

	inline std::string StripString(const std::string& aStringToStrip, const char aCharToStrip, bool aStripLeading = true, bool aStripTrailing = true)
	{
		const size_t startingPos = aStripLeading ? aStringToStrip.find_first_not_of(aCharToStrip) : 0;
		const size_t endingPos = aStripTrailing ? aStringToStrip.find_last_not_of(aCharToStrip) : aStringToStrip.size();

		return aStringToStrip.substr(startingPos, endingPos - startingPos + 1);
	}

	inline void ReplaceInString(std::string& aString, const std::string& aToReplace, const std::string& aReplacement)
	{
		size_t pos = 0;
		while ((pos = aString.find(aToReplace, pos)) != std::string::npos)
		{
			aString.replace(pos, aToReplace.size(), aReplacement);
			//pos += aReplacement.size();
		}
	}

	inline void SplitStringBy(const std::string& aString, const char aDelimiter, std::vector<std::string>& aOut)
	{

		const size_t findPos = aString.find_first_not_of(aDelimiter);
		if (findPos == std::string::npos) { return; } //if the string is just delimiters, we don't want to do anything with it.

		size_t startingPos = findPos + 1; //in case we have leading delimiters, for example leading whitespace.

		std::string workingString = StripString(aString, aDelimiter);
		size_t pos = 0;
		while ((pos = workingString.find(aDelimiter)) != std::string::npos)
		{

			const std::string stripped = StripString(workingString.substr(0, pos), aDelimiter);
			if (!stripped.empty())
			{
				aOut.push_back(stripped);
			}
			workingString.erase(0, pos + 1);
		}
		const std::string stripped = StripString(workingString, aDelimiter);
		if (!stripped.empty())
		{
			aOut.push_back(stripped);
		}
	}

	inline Vector3f GetColour(const std::string& aColourString)
	{
		//we'll be inputting colours in this format: "#ff5100"
		int r, g, b;
		sscanf_s(aColourString.c_str(), "#%02x%02x%02x", &r, &g, &b);

		return { (float)r / 255.0f,(float)g / 255.0f ,(float)b / 255.0f };
	}
}

namespace KE
{
	struct StructDefinition;

	struct DataType
	{
		std::string typeName;
		Vector3f typeColour;
		bool isStruct = false;
	};

	struct Parameter
	{
		DataType type;
		std::string name;
	};

	struct FunctionDefinition
	{
		std::string name;
		std::string operatorKey;

		struct Variant
		{
			DataType returnType;
			std::vector<Parameter> parameters;
		};

		std::vector<Variant> variants;
	};

	struct StructDefinition
	{
		std::string name;
		std::vector<Parameter> members;
	};

	struct BufferDefinition
	{
		std::string name;
		std::vector<Parameter> members;
		unsigned int slot;
	};

	struct EntrypointDefinition
	{
		std::string name;

		std::vector<Parameter> inputParameters;
		DataType returnType;
	};

	struct TextureDefinition
	{
		DataType type;
		std::string name;
		unsigned int slot;
	};

	struct LanguageDefinition
	{
		std::string langdef;
		std::string funcdef;

		std::unordered_map<std::string, StructDefinition> structs;
		std::unordered_map<std::string, BufferDefinition> buffers;
		std::unordered_map<std::string, FunctionDefinition> functions;
		std::unordered_map<std::string, DataType> dataTypes;
		std::unordered_map<std::string, EntrypointDefinition> entrypoints;
		std::unordered_map<std::string, TextureDefinition> textures;
	};

	struct LanguageDefinitionNew
	{

	};

	template <typename AnnotationType>
	struct AnnotatedCode
	{
		AnnotationType annotation;
		std::vector<std::string> annotationParameters;

		std::vector<std::string> code;
	};

	class LanguageInspector
	{
	public:
		static bool InspectType(const DataType& aType, void* aData, const char* aLabel);
	};

	class LanguageDefiner
	{
	private:
		static bool IsType(const std::string& aWord, const LanguageDefinition& aLanguage);
		static bool IsFunctionDefinition(const std::string& aLine);

		static void EvaluateStructDefinition(const std::string& aLine, StructDefinition* aStruct, const LanguageDefinition& aWorkingLanguage);
		static void EvaluateFunctionDefinition(const std::string& aLine, FunctionDefinition* aFunction, const LanguageDefinition& aWorkingLanguage);

	public:
		static void InterpretStructs(const char* aStructsFile, LanguageDefinition& aWorkingLanguage);
		static void InterpretBuffers(const char* aBuffersFile, LanguageDefinition& aWorkingLanguage);
		static void InterpretFunctions(const char* aFunctionsFile, LanguageDefinition& aWorkingLanguage);
		static void InterpretEntrypoints(const char* aEntrypointsFile, LanguageDefinition& aWorkingLanguage);
		static void InterpretTextures(const char* aTexturesFile, LanguageDefinition& aWorkingLanguage);

		static LanguageDefinition DefineLanguage(const char* aLanguageDefinitionFile);
	};

	template <typename DefinerClass>
	class LanguageDefinerNew
	{
	private:


	public:
		static DefinerMember::LangDefinition Define(const char* aLanguageDefinitionFile);

	};

	template <typename DefinerClass>
	DefinerMember::LangDefinition LanguageDefinerNew<DefinerClass>::Define(const char* aLanguageDefinitionFile)
	{
		typename DefinerClass::LangDefinition out;

		std::string contents = GetFileContents(aLanguageDefinitionFile);


		nlohmann::json languageDefinition = nlohmann::json::parse(contents);

		for (const auto& dataType : languageDefinition.at("dataTypes").items())
		{
			const std::string key = dataType.key();
			const std::string colStr = languageDefinition["typeColours"][key].get<std::string>();

			out.dataTypes[key] = {
				key,
				GetColour(colStr)
			};
		}

		const std::string annotationFormat = languageDefinition.at("annotationFormat").get<std::string>();

		std::unordered_map<std::string, DefinerMember::Annotations> annotations;
		DefinerClass::RegisterAnnotations(annotations);

		for (const auto& functionFile : languageDefinition.at("functionDefinitionFiles"))
		{
			std::string funcFile = functionFile.get<std::string>();

			std::string funcFileContents = GetFileContents(funcFile.c_str());
			std::stringstream fileStream = std::stringstream(funcFileContents);

			std::string line;
						
			std::string annotation = "";

			AnnotatedCode<DefinerMember::Annotations> annotatedCode;

			while (std::getline(fileStream, line))
			{
				if (line.find(annotationFormat) != std::string::npos)
				{
					if (!annotation.empty()) //we've hit an annotation, so we must send our current one to be interpreted first.
					{
						DefinerClass::Interpret(out, annotatedCode);
					}

					annotatedCode = {};
					annotation = line;
					SplitStringBy(annotation, ' ', annotatedCode.annotationParameters);
					annotatedCode.annotation = annotations[annotatedCode.annotationParameters[0]];
				}
				else
				{
					annotatedCode.code.push_back(line);
				}
			}

		}
		return out;

	}
}
