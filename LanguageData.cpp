#include "stdafx.h"
#include "LanguageData.h"

#include "CodeNode.h"
#include "NodeDatabase.h"
#include "imgui/imgui.h"
#include "nlohmann/json.hpp"


bool KE::LanguageInspector::InspectType(const DataType& aType, void* aData, const char* aLabel)
{
	if (aType.typeName == "float") {  return ImGui::DragFloat(aLabel, (float*)aData); }
	if (aType.typeName == "float2") { return ImGui::DragFloat2(aLabel, (float*)aData); }
	if (aType.typeName == "float3") { return ImGui::DragFloat3(aLabel, (float*)aData); }
	if (aType.typeName == "float4") { return ImGui::DragFloat4(aLabel, (float*)aData); }

	if (aType.typeName == "int") { return ImGui::DragInt(aLabel, (int*)aData); }
	if (aType.typeName == "int2") { return ImGui::DragInt2(aLabel, (int*)aData); }
	if (aType.typeName == "int3") { return ImGui::DragInt3(aLabel, (int*)aData); }
	if (aType.typeName == "int4") { return ImGui::DragInt4(aLabel, (int*)aData); }

	if (aType.typeName == "bool") { return ImGui::Checkbox(aLabel, (bool*)aData); }
	if (aType.typeName == "uint") { return ImGui::DragInt(aLabel, (int*)aData, 1, 0, INT_MAX, "%d", ImGuiSliderFlags_AlwaysClamp); }

	return false;
}

bool KE::LanguageDefiner::IsType(const std::string& aWord, const LanguageDefinition& aLanguage)
{
	if (aLanguage.dataTypes.contains(aWord))
	{
		return true;
	}

	return false; //TODO: this should be false, but I'm just testing the function right now
}

bool KE::LanguageDefiner::IsFunctionDefinition(const std::string& aLine)
{
	return false;
}

void KE::LanguageDefiner::EvaluateStructDefinition(const std::string& aLine, StructDefinition* aStruct, const LanguageDefinition& aWorkingLanguage)
{

}

void KE::LanguageDefiner::EvaluateFunctionDefinition(const std::string& aLine, FunctionDefinition* aFunction, const LanguageDefinition& aWorkingLanguage)
{

	//identify input parameters
	const size_t openParen = aLine.find('(');
	const size_t closeParen = aLine.find(')', openParen);

	const std::string parameters = aLine.substr(openParen + 1, closeParen - openParen - 1);
	std::vector<std::string> splitParameters;
	SplitStringBy(parameters, ',', splitParameters);

	for (const auto& parameter : splitParameters)
	{
		//split by space
		std::vector<std::string> parameterParts;
		SplitStringBy(parameter, ' ', parameterParts);

		aFunction->variants.back().parameters.push_back({ aWorkingLanguage.dataTypes.at(parameterParts[0]), parameterParts[1]});
	}

	//identify function return type, it should be the first word in the line that is a type
	std::vector<std::string> lineParts;
	SplitStringBy(aLine, ' ', lineParts);
	for (const auto& part : lineParts)
	{
		if (IsType(part, aWorkingLanguage))
		{
			aFunction->variants.back().returnType = aWorkingLanguage.dataTypes.at(part);
			break;
		}
	}
}

void KE::LanguageDefiner::InterpretStructs(const char* aStructsFile, LanguageDefinition& aWorkingLanguage)
{
	const std::string structFileContents = GetFileContents(aStructsFile);
	std::stringstream fileStream = std::stringstream(structFileContents);

	//the file we're reading is just a source code file, so we'll have to do some parsing to get the struct definitions
	std::string line;
	//std::vector<std::string> lines;

	StructDefinition* currentStruct = nullptr;
	size_t instructLine = 0;
	size_t scopeDepth = 0;

	while (std::getline(fileStream, line))
	{
		if (!currentStruct)
		{
			const size_t structInfoPosition = line.find("//@struct");
			if (structInfoPosition != std::string::npos)
			{
				const size_t nameStart = structInfoPosition + sizeof("//@struct");
				const size_t nameEnd = line.find(' ', nameStart);

				const std::string structName = line.substr(nameStart, nameEnd - nameStart);

				aWorkingLanguage.dataTypes[structName] = {structName, {1.0f, 0.0f, 1.0f}, true};
				aWorkingLanguage.structs[structName] = {};
				currentStruct = &aWorkingLanguage.structs[structName];

				currentStruct->name = structName;

				instructLine = 0;
			}
		}
		else
		{
			if (instructLine == 0)
			{
				EvaluateStructDefinition(line, currentStruct, aWorkingLanguage);
			}

			if (line.find('{') != std::string::npos)
			{
				++scopeDepth;
			}

			if (scopeDepth > 0)
			{
				std::vector<std::string> parts;
				ReplaceInString(line, "  ", " ");
				SplitStringBy(line, ' ', parts);
				if (parts.size() >= 2 && IsType(parts[0], aWorkingLanguage))
				{
					currentStruct->members.push_back({ aWorkingLanguage.dataTypes.at(parts[0]), parts[1] });
				}
			}

			if (line.find('}') != std::string::npos)
			{
				--scopeDepth;
				if (scopeDepth == 0)
				{
					currentStruct = nullptr;
				}
			}

			instructLine++;
		}
	}
	//

}

void KE::LanguageDefiner::InterpretBuffers(const char* aBuffersFile, LanguageDefinition& aWorkingLanguage)
{
	

	const std::string buffersFileContents = GetFileContents(aBuffersFile);
	std::stringstream fileStream = std::stringstream(buffersFileContents);

	//the file we're reading is just a source code file, so we'll have to do some parsing to get the struct definitions
	std::string line;
	//std::vector<std::string> lines;

	BufferDefinition* currentBuffer = nullptr;
	size_t instructLine = 0;
	size_t scopeDepth = 0;
	unsigned int bufferSlot = 0;

	while (std::getline(fileStream, line))
	{
		if (!currentBuffer)
		{
			const size_t bufferInfoPosition = line.find("//@buffer");
			if (bufferInfoPosition != std::string::npos)
			{
				const size_t nameStart = bufferInfoPosition + sizeof("//@buffer");
				const size_t nameEnd = line.find(' ', nameStart);

				const std::string bufferName = line.substr(nameStart, nameEnd - nameStart);

				aWorkingLanguage.dataTypes[bufferName] = { bufferName, {1.0f, 0.0f, 1.0f}, true };
				aWorkingLanguage.buffers[bufferName] = {};
				currentBuffer = &aWorkingLanguage.buffers[bufferName];

				currentBuffer->name = bufferName;

				instructLine = 0;
				bufferSlot = 0;
			}
		}
		else
		{
			if (instructLine == 0)
			{
				const size_t slotPos = line.find("register(");
				if (slotPos != std::string::npos)
				{
					const size_t slotStart = slotPos + sizeof("register(");
					const size_t slotEnd = line.find(')', slotStart);

					const std::string slotStr = line.substr(slotStart, slotEnd - slotStart);
					bufferSlot = (unsigned int)std::stoi(slotStr);
					currentBuffer->slot = bufferSlot;
				}
			}

			if (line.find('{') != std::string::npos)
			{
				++scopeDepth;
			}

			if (scopeDepth > 0)
			{
				std::vector<std::string> parts;
				ReplaceInString(line, "  ", " ");
				SplitStringBy(line, ' ', parts);
				if (parts.size() >= 2 && IsType(parts[0], aWorkingLanguage))
				{
					currentBuffer->members.push_back({ aWorkingLanguage.dataTypes.at(parts[0]), parts[1] });
				}
			}

			if (line.find('}') != std::string::npos)
			{
				--scopeDepth;
				if (scopeDepth == 0)
				{
					currentBuffer = nullptr;
				}
			}

			instructLine++;
		}
	}
	//
}

void KE::LanguageDefiner::InterpretFunctions(const char* aFunctionsFile, LanguageDefinition& aWorkingLanguage)
{
	const std::string functionFileContents = GetFileContents(aFunctionsFile);
	std::stringstream fileStream = std::stringstream(functionFileContents);

	std::string line;

	FunctionDefinition* currentFunction = nullptr;
	size_t inFunctionLine = 0;
	size_t scopeDepth = 0;

	while (std::getline(fileStream, line))
	{
		if (!currentFunction)
		{
			const size_t functionInfoPosition = line.find("//@function");
			if (functionInfoPosition != std::string::npos)
			{
				const size_t nameStart = functionInfoPosition + sizeof("//@function");
				const size_t nameEnd = line.find(' ', nameStart);

				const std::string funcName = line.substr(nameStart, nameEnd - nameStart);

				if (!aWorkingLanguage.functions.contains(funcName))
				{
					aWorkingLanguage.functions[funcName] = {};
					aWorkingLanguage.functions[funcName].name = funcName;
					
				}
				currentFunction = &aWorkingLanguage.functions[funcName];
				currentFunction->variants.push_back({});

				inFunctionLine = 0;
			}
		}
		else
		{
			if (inFunctionLine == 0)
			{
				EvaluateFunctionDefinition(line, currentFunction, aWorkingLanguage);
			}

			if (line.find('{') != std::string::npos)
			{
				++scopeDepth;
			}

			if (line.find('}') != std::string::npos)
			{
				--scopeDepth;
				if (scopeDepth == 0)
				{
					currentFunction = nullptr;
				}
			}

			inFunctionLine++;
		}
	}
	//
}

void KE::LanguageDefiner::InterpretEntrypoints(const char* aEntrypointsFile, LanguageDefinition& aWorkingLanguage)
{
	const std::string entrypointsFileContents = GetFileContents(aEntrypointsFile);
	std::stringstream fileStream = std::stringstream(entrypointsFileContents);

	//the file we're reading is just a source code file, so we'll have to do some parsing to get the function definitions
	std::string line;

	EntrypointDefinition* currentEntrypoint = nullptr;

	while (std::getline(fileStream, line))
	{
		if (!currentEntrypoint)
		{
			const size_t entryPointPos = line.find("//@entrypoint");
			if (entryPointPos != std::string::npos)
			{
				const size_t nameStart = entryPointPos + sizeof("//@entrypoint");
				const size_t nameEnd = line.find(' ', nameStart);

				const std::string entrypointName = line.substr(nameStart, nameEnd - nameStart);

				aWorkingLanguage.entrypoints[entrypointName] = {};
				currentEntrypoint = &aWorkingLanguage.entrypoints[entrypointName];

				currentEntrypoint->name = entrypointName;

			}
		}
		else
		{
			std::vector<std::string> words;
			std::string workingLine = line;

			//replace parentheses with spaces so we can split the line by spaces
			while (workingLine.find('(') != std::string::npos)
			{
				workingLine.replace(workingLine.find('('), 1, " ");
			}
			while (workingLine.find(')') != std::string::npos)
			{
				workingLine.replace(workingLine.find(')'), 1, " ");
			}

			ReplaceInString(line, "  ", " ");
			SplitStringBy(workingLine, ' ', words);

			bool assignedReturnType = false;
			for (size_t i = 0; i < words.size(); i++)
			{
				const auto& word = words[i];
				if (IsType(word, aWorkingLanguage))
				{
					if (!assignedReturnType)
					{
						currentEntrypoint->returnType = aWorkingLanguage.dataTypes.at(word);
						assignedReturnType = true;
					}
					else
					{
						currentEntrypoint->inputParameters.push_back({ aWorkingLanguage.dataTypes.at(word), words[i+1]});
					}
				}
			}

			currentEntrypoint = nullptr;
		}
	}
}

void KE::LanguageDefiner::InterpretTextures(const char* aTexturesFile, LanguageDefinition& aWorkingLanguage)
{
	const std::string texturesFileContents = GetFileContents(aTexturesFile);
	std::stringstream fileStream = std::stringstream(texturesFileContents);

	//the file we're reading is just a source code file, so we'll have to do some parsing to get the function definitions
	std::string line;

	TextureDefinition* currentTexture = nullptr;
	size_t instructLine = 0;
	unsigned int textureSlot = 0;

	while (std::getline(fileStream, line))
	{
		if (!currentTexture)
		{
			const size_t texturePos = line.find("//@texture2D");
			if (texturePos != std::string::npos)
			{
				const size_t nameStart = texturePos + sizeof("//@texture2D");
				const size_t nameEnd = line.find(' ', nameStart);

				const std::string textureName = line.substr(nameStart, nameEnd - nameStart);

				aWorkingLanguage.textures[textureName] = {};
				currentTexture = &aWorkingLanguage.textures[textureName];

				currentTexture->name = textureName;
				instructLine = 0;
				textureSlot = 0;
			}
		}
		else
		{
			if (instructLine == 0)
			{
				size_t slotPos = line.find("register(");
				if (slotPos != std::string::npos)
				{
					const size_t slotStart = slotPos + sizeof("register(");
					const size_t slotEnd = line.find(')', slotStart);

					const std::string slotStr = line.substr(slotStart, slotEnd - slotStart);
					textureSlot = (unsigned int)std::stoi(slotStr);
					currentTexture->slot = textureSlot;
				}

				currentTexture = nullptr;
			}
			instructLine++;
		}
	}
}

KE::LanguageDefinition KE::LanguageDefiner::DefineLanguage(const char* aLanguageDefinitionFile)
{
	LanguageDefinition out;

	out.langdef = GetFileContents(aLanguageDefinitionFile);

	nlohmann::json languageDefinition = nlohmann::json::parse(out.langdef);

	for (const auto& dataType : languageDefinition.at("dataTypes").items())
	{
		const std::string key = dataType.key();
		const std::string colStr = languageDefinition["typeColours"][key].get<std::string>();

		out.dataTypes[key] = {
			key,
			GetColour(colStr)
		};
	}

	for (const auto& functionFile : languageDefinition.at("functionDefinitionFiles"))
	{
		std::string funcFile = functionFile.get<std::string>();

		InterpretFunctions(funcFile.c_str(), out);
		InterpretStructs(funcFile.c_str(), out);

		InterpretBuffers(funcFile.c_str(), out);
		InterpretTextures(funcFile.c_str(), out);

		InterpretEntrypoints(funcFile.c_str(), out);
	}
	return out;
}
