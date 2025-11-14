#include "stdafx.h"
#include "HLSLDefiner.h"


void KE::HLSLDefiner::RegisterAnnotations(std::unordered_map<std::string, Annotations>& anAnnotationMap)
{
	anAnnotationMap["//@entrypoint"] = Annotations::Entrypoint;
	anAnnotationMap["//@function"]   = Annotations::Function;
	anAnnotationMap["//@struct"]     = Annotations::Struct;
	anAnnotationMap["//@buffer"]     = Annotations::Buffer;
	anAnnotationMap["//@texture2D"]  = Annotations::Texture2D;
	anAnnotationMap["//@intrinsic"]  = Annotations::Intrinsic;
	anAnnotationMap["//@operator"]   = Annotations::Operator;
}

void KE::HLSLDefiner::Interpret(LangDefinition& aDefinition, const AnnotatedCode<Annotations>& aCode)
{
	if (aCode.code.empty()) { return; }

	switch (aCode.annotation)
	{
	case Annotations::Entrypoint:
	{
		std::vector<std::string> entrypointWords;
		std::string line = aCode.code[0];
		ReplaceInString(line, "(", " ");
		ReplaceInString(line, ")", " ");
		ReplaceInString(line, "  ", " ");
		SplitStringBy(line, ' ', entrypointWords);

		const std::string& entrypointName = aCode.annotationParameters[1];
		auto& entry = aDefinition.entrypoints[entrypointName];
		entry.returnType = aDefinition.dataTypes.at(entrypointWords[0]);
		entry.name = entrypointName; // use the annotated name, so it can be named independently of the output code :)

		for (int i = 2; i < entrypointWords.size(); i++)
		{
			if (!aDefinition.dataTypes.contains(entrypointWords[i])) { continue; }

			entry.inputParameters.push_back({ aDefinition.dataTypes.at(entrypointWords[i]), entrypointWords[i + 1] });
		}

		break;
	}
	case Annotations::Function:
	{
		std::vector<std::string> functionWords;
		std::string line = aCode.code[0];
		ReplaceInString(line, "(", " ");
		ReplaceInString(line, ")", " ");
		ReplaceInString(line, ",", " ");
		ReplaceInString(line, "  ", " ");

		SplitStringBy(line, ' ', functionWords);

		auto& function = aDefinition.functions[functionWords[1]];
		auto& variant = function.variants.emplace_back();
		variant.returnType = aDefinition.dataTypes.at(functionWords[0]);
		function.name = aCode.annotationParameters[1]; // use the annotated name, so it can be named independently of the output code :)

		for (int i = 2; i < functionWords.size(); i++)
		{
			if (!aDefinition.dataTypes.contains(functionWords[i])) { continue; }

			variant.parameters.push_back({ aDefinition.dataTypes.at(functionWords[i]), functionWords[i + 1] });
		}
		break;
	}
	case Annotations::Struct:
	{

		const std::string& structName = aCode.annotationParameters[1];
		aDefinition.dataTypes[structName] = { structName, {1.0f, 0.0f, 1.0f}, true };
		auto& currentStruct = aDefinition.structs[structName];
		currentStruct.name = structName;

		for (const auto& line : aCode.code)
		{
			if (line.find("struct") != std::string::npos ||
				line.find('{') != std::string::npos ||
				line.find('}') != std::string::npos
				) { continue; }

			std::string workingLine = line;

			std::vector<std::string> lineParts;
			ReplaceInString(workingLine, "  ", " ");
			SplitStringBy(workingLine, ' ', lineParts);

			if (lineParts.size() >= 2 && aDefinition.dataTypes.contains(lineParts[0]))
			{
				currentStruct.members.push_back({ aDefinition.dataTypes.at(lineParts[0]), lineParts[1] });
			}
		}


		break;
	}
	case Annotations::Buffer:
	{
		const std::string& bufferName = aCode.annotationParameters[1];
		aDefinition.dataTypes[bufferName] = { bufferName, {1.0f, 0.0f, 1.0f}, true };
		auto& currentBuffer = aDefinition.buffers[bufferName];
		currentBuffer.name = bufferName;

		for (const auto& line : aCode.code)
		{

			if (line.find('{') != std::string::npos ||
				line.find('}') != std::string::npos)
			{
				continue;
			}

			if (const size_t registerPos = line.find("register"); registerPos != std::string::npos)
			{
				const size_t parenBegin = line.find_first_of('(', registerPos);
				const size_t parenEnd = line.find_first_of(')', parenBegin);

				currentBuffer.slot = std::stoi(line.substr(parenBegin + 2, (parenEnd - parenBegin) - 2));
			}

			std::string workingLine = line;

			std::vector<std::string> lineParts;
			ReplaceInString(workingLine, "  ", " ");
			SplitStringBy(workingLine, ' ', lineParts);

			if (lineParts.size() >= 2 && aDefinition.dataTypes.contains(lineParts[0]))
			{
				currentBuffer.members.push_back({ aDefinition.dataTypes.at(lineParts[0]), lineParts[1] });
			}
		}

		break;
	}
	case Annotations::Texture2D:
	{
		const std::string& textureName = aCode.annotationParameters[1];
		aDefinition.dataTypes[textureName] = { textureName, {1.0f, 0.0f, 1.0f}, true };
		auto& currentTexture = aDefinition.textures[textureName];
		currentTexture.name = textureName;
		currentTexture.type = aDefinition.dataTypes.at("Texture2D");
		const std::string& line = aCode.code[0];

		if (const size_t registerPos = line.find("register"); registerPos != std::string::npos)
		{
			const size_t parenBegin = line.find_first_of('(', registerPos);
			const size_t parenEnd = line.find_first_of(')', parenBegin);

			currentTexture.slot = std::stoi(line.substr(parenBegin + 2, (parenEnd - parenBegin) - 2));
		}

		break;
	}
	case Annotations::Intrinsic:
	{
		const std::string& intrinsicName = aCode.annotationParameters[1];

		std::string outputs = aCode.code[0];
		ReplaceInString(outputs, "  ", " ");

		std::vector<std::string> inputs;
		for (int i = 1; i < aCode.code.size(); i++)
		{
			if (!aCode.code[i].starts_with("//#")) { break; }

			inputs.push_back(aCode.code[i]);
			ReplaceInString(inputs.back(), "  ", " ");
		}

		std::vector<std::string> outputTypes;
		std::vector<std::vector<std::string>> inputTypes;

		SplitStringBy(outputs, ' ', outputTypes);
		for (int i = 0; i < inputs.size(); i++)
		{
			SplitStringBy(inputs[i], ' ', inputTypes.emplace_back());
		}


		aDefinition.functions[intrinsicName].name = intrinsicName;

		for (int i = 1; i < outputTypes.size(); i++)
		{
			auto& function = aDefinition.functions[intrinsicName].variants.emplace_back();
			function.returnType = aDefinition.dataTypes[outputTypes[i]];

			for (int j = 0; j < inputTypes.size(); j++)
			{
				auto& param = function.parameters.emplace_back();
				const std::string& paramName = inputTypes[j][1];
				const std::string& paramType = inputTypes[j][i + 1];

				param.name = paramName;
				param.type = aDefinition.dataTypes[paramType];
			}
		}

		break;
	}
	case Annotations::Operator:
	{
		const std::string& operatorName = aCode.annotationParameters[1];
		const std::string& operatorKey = aCode.annotationParameters[2];

		std::string outputs = aCode.code[0];
		ReplaceInString(outputs, "  ", " ");

		std::vector<std::string> inputs;
		for (int i = 1; i < aCode.code.size(); i++)
		{
			if (!aCode.code[i].starts_with("//#")) { break; }

			inputs.push_back(aCode.code[i]);
			ReplaceInString(inputs.back(), "  ", " ");
		}

		std::vector<std::string> outputTypes;
		std::vector<std::vector<std::string>> inputTypes;

		SplitStringBy(outputs, ' ', outputTypes);
		for (int i = 0; i < inputs.size(); i++)
		{
			SplitStringBy(inputs[i], ' ', inputTypes.emplace_back());
		}

		aDefinition.functions[operatorName].name = operatorName;
		aDefinition.functions[operatorName].operatorKey = operatorKey;

		for (int i = 1; i < outputTypes.size(); i++)
		{
			auto& function = aDefinition.functions[operatorName].variants.emplace_back();
			function.returnType = aDefinition.dataTypes[outputTypes[i]];

			for (int j = 0; j < inputTypes.size(); j++)
			{
				auto& param = function.parameters.emplace_back();
				const std::string& paramName = inputTypes[j][1];
				const std::string& paramType = inputTypes[j][i + 1];

				param.name = paramName;
				param.type = aDefinition.dataTypes[paramType];
			}
		}

		break;
	}
	}
}
