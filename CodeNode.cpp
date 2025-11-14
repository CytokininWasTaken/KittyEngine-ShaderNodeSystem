#include "stdafx.h"
#include "CodeNode.h"

#include "Graphics/Graphics.h"
#include "imgui/imgui.h"
#include "Script.h"

#include <d3d11.h>

#include "HLSLDefiner.h"

#pragma region CodeFunctionNode

void KE::CodeFunctionNode::AddData(FunctionDefinition aFunction)
{
	myFunctionDefinition = aFunction;
}

void KE::CodeFunctionNode::Init()
{
	for (const auto& param : myFunctionDefinition.variants[myVariantIndex].parameters)
	{
		AddInputPin(ValueType::CodeValue, PinType::Value, param.name.c_str());
		codePinData.push_back({ CodePinType::Value, {param.type} });
	}
	AddOutputPin(ValueType::CodeValue, PinType::OutputValue, "Output");
	codePinData.push_back({ CodePinType::Value, {myFunctionDefinition.variants[myVariantIndex].returnType} });

	for (int i = 0; i < myFunctionDefinition.variants[myVariantIndex].parameters.size(); i++)
	{
		inputPins[i].codeData = &codePinData[i];
	}
	outputPins[0].codeData = &codePinData.back();
}

KE::CommandList KE::CodeFunctionNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	//generate the function call
	GenerateVariableCommand* functionCall = commands.Add<GenerateVariableCommand>();
	functionCall->origin = outputPins[0].ID;
	functionCall->type = myFunctionDefinition.variants[myVariantIndex].returnType;
	functionCall->mutability = CodeVariableMutability::Variable;

	if (myFunctionDefinition.operatorKey == "")
	{
		std::vector<ScriptMemberID> functionCallIDS;
		for (int i = 0; i < myFunctionDefinition.variants[myVariantIndex].parameters.size(); i++)
		{
			functionCallIDS.push_back(inputPins[i].ID);
		}
		functionCall->data.assignmentInputs.push_back({ functionCallIDS, CodeOperation::CallFunction});
		functionCall->data.optionalData = myFunctionDefinition.name;
	}
	else
	{
		functionCall->data.assignmentInputs.push_back({ inputPins[0].ID, StringToCodeMathOperation(myFunctionDefinition.operatorKey) });
		functionCall->data.assignmentInputs.push_back({ inputPins[1].ID, CodeOperation::None });
	}

	return commands;
}

void KE::CodeFunctionNode::SetVariantIndex(unsigned aIndex)
{
	//if (myFunctionDefinition.variants.empty()) { return; }

	myVariantIndex = aIndex;

	auto& newFunctionDefinition = myFunctionDefinition.variants[aIndex];

	for (int i = 0; i < newFunctionDefinition.parameters.size(); i++)
	{
		((CodePin*)inputPins[i].codeData)->value.type = newFunctionDefinition.parameters[i].type;
	}

	((CodePin*)outputPins[0].codeData)->value.type = newFunctionDefinition.returnType;
}

void KE::CodeFunctionNode::ExtraSerialize(void* aOutJson, Script* aLoadingScript)
{
	nlohmann::json* json = (nlohmann::json*)aOutJson;
	(*json)["variantIndex"] = static_cast<int>(myVariantIndex);
}

void KE::CodeFunctionNode::ExtraDeserialize(void* aInJson, Script* aLoadingScript)
{
	nlohmann::json* json = (nlohmann::json*)aInJson;
	myVariantIndex = (*json)["variantIndex"];
	SetVariantIndex(myVariantIndex);
}

#pragma endregion

#pragma region CodeStructNode

void KE::CodeStructNode::AddData(StructDefinition aStruct)
{
	myStructDefinition = aStruct;
}

void KE::CodeStructNode::Init()
{
	for (const auto& param : myStructDefinition.members)
	{
		AddOutputPin(ValueType::CodeValue, PinType::OutputValue, param.name.c_str());
		codePinData.push_back({ CodePinType::Value, { param.type } });
	}

	for (int i = 0; i < myStructDefinition.members.size(); i++)
	{
		outputPins[i].codeData = &codePinData[i];
	}
}

KE::CommandList KE::CodeStructNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	for (int i = 0; i < myStructDefinition.members.size(); i++)
	{
		GenerateVariableCommand* memberVariable = commands.Add<GenerateVariableCommand>();
		memberVariable->origin = outputPins[i].ID;
		memberVariable->data.level = CodeVariableLevel::Register;
		memberVariable->data.optionalData = myStructDefinition.members[i].name;
				
		memberVariable->type = myStructDefinition.members[i].type;
		memberVariable->mutability = CodeVariableMutability::Constant;
	}

	return commands;
}
#pragma endregion

#pragma region CodeBufferNode
void KE::CodeBufferNode::AddData(BufferDefinition aStruct)
{
	myBufferDefinition = aStruct;
}

void KE::CodeBufferNode::CustomRender()
{
	ImGui::Text("Slot: %d", myBufferDefinition.slot);
}

void KE::CodeBufferNode::Init()
{
	for (const auto& param : myBufferDefinition.members)
	{
		AddOutputPin(ValueType::CodeValue, PinType::OutputValue, param.name.c_str());
		codePinData.push_back({ CodePinType::Value, { param.type } });
	}

	for (int i = 0; i < myBufferDefinition.members.size(); i++)
	{
		outputPins[i].codeData = &codePinData[i];
	}
}

KE::CommandList KE::CodeBufferNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	for (int i = 0; i < myBufferDefinition.members.size(); i++)
	{
		GenerateVariableCommand* memberVariable = commands.Add<GenerateVariableCommand>();
		memberVariable->origin = outputPins[i].ID;
		memberVariable->data.level = CodeVariableLevel::Register;
		memberVariable->data.optionalData = myBufferDefinition.members[i].name;

		memberVariable->type = myBufferDefinition.members[i].type;
		memberVariable->mutability = CodeVariableMutability::Constant;
	}

	return commands;
}
#pragma endregion

#pragma region CodeTextureNode
void KE::CodeTextureNode::AddData(TextureDefinition aTexture)
{
	myTextureDefinition = aTexture;
}

void KE::CodeTextureNode::CustomRender()
{
	//ImGui::Text("Slot: %d", myTextureDefinition.slot);
	//
	//ID3D11ShaderResourceView* srv = parentScript->GetCodeRenderingContext()->shaderTextures[myTextureDefinition.slot];
	//if (srv)
	//{
	//	ImGui::Image(srv, ImVec2(128, 128));
	//}
}

void KE::CodeTextureNode::Init()
{
	AddOutputPin(ValueType::CodeValue, PinType::OutputValue, "Output");
	codePinData.push_back({ CodePinType::Value, { myTextureDefinition.type } });

	outputPins[0].codeData = &codePinData[0];
}

KE::CommandList KE::CodeTextureNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	GenerateVariableCommand* memberVariable = commands.Add<GenerateVariableCommand>();
	memberVariable->origin = outputPins[0].ID;
	memberVariable->data.level = CodeVariableLevel::Register;
	memberVariable->data.optionalData = myTextureDefinition.name;

	memberVariable->type = myTextureDefinition.type;
	memberVariable->mutability = CodeVariableMutability::Constant;

	return commands;
}
#pragma endregion

#pragma region CodeEntryPointNode
void KE::CodeEntryPointNode::AddData(const EntrypointDefinition& aEntrypoint)
{
	myEntrypointDefinition = aEntrypoint;
	myName = "Enter " + aEntrypoint.name;
}

auto KE::CodeEntryPointNode::GetParameters() const
{
	std::vector<std::pair<const DataType*, std::pair<std::string,std::string>>> inputParams;
	for (const auto& param : myEntrypointDefinition.inputParameters)
	{
		if (param.type.isStruct)
		{
			auto* hlslDef = (HLSLDefiner::LangDefinition*)parentScript->GetLanguageDefinition();

			for (const auto& member : hlslDef->structs[param.type.typeName].members)
			{
				inputParams.push_back({ &member.type, {param.name, member.name} });
			}
		}
		else
		{
			inputParams.push_back({ &param.type, {"", param.name} });
		}
	}

	return inputParams;
}

void KE::CodeEntryPointNode::Init()
{
	AddOutputPin(ValueType::FlowValue, PinType::Flow, "");

	const auto& params = GetParameters();

	for (const auto& param : params)
	{
		AddOutputPin(ValueType::CodeValue, PinType::OutputValue, param.second.second.c_str());
		codePinData.push_back({ CodePinType::Value, { *param.first } });
	}

	for (int i = 0; i < params.size(); i++)
	{
		outputPins[i+1].codeData = &codePinData[i];
	}
}

KE::CommandList KE::CodeEntryPointNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	CreateScopeCommand* scope = commands.Add<CreateScopeCommand>();
	scope->scopeOwnerID = outputPins[0].ID;

	const auto& params = GetParameters();

	for (int i = 0; i < params.size(); i++)
	{
		scope->pinList.pins.push_back(outputPins[i + 1].ID);
	}

	std::string inputParams;

	for (int i = 0; i < myEntrypointDefinition.inputParameters.size(); i++)
	{
		if (i != 0) { inputParams += ", "; }
		inputParams += std::format("{} {}", myEntrypointDefinition.inputParameters[i].type.typeName, myEntrypointDefinition.inputParameters[i].name);
	}

	const std::string formatted = std::format(
		"{} {}({})\n{}\n  {} OUTPUT;\n",
		myEntrypointDefinition.returnType.typeName,
		"main",//myEntrypointDefinition.name,
		inputParams,
		"{",
		myEntrypointDefinition.returnType.typeName
	);

	scope->scopeStart = formatted;
	
	scope->scopeEnd = "  return OUTPUT;\n}\n";


	for (int i = 0; i < params.size(); i++)
	{
		GenerateVariableCommand* inputVariable = commands.Add<GenerateVariableCommand>();
		inputVariable->origin = outputPins[i+1].ID;
		inputVariable->type = *params[i].first;
		inputVariable->mutability = CodeVariableMutability::Variable;
		inputVariable->data.level = CodeVariableLevel::Register;
		inputVariable->data.optionalData =  params[i].second.first == "" ? 
			                                    params[i].second.second :
			                                    std::format("{}.{}",params[i].second.first, params[i].second.second);
	}

	return commands;
}
#pragma endregion

#pragma region CodeExitPointNode
void KE::CodeExitPointNode::AddData(const EntrypointDefinition& aEntrypoint)
{
	myEntrypointDefinition = aEntrypoint;
	myName = "Exit " + aEntrypoint.name;
}

auto KE::CodeExitPointNode::GetParameters() const
{
	std::vector<std::pair<const DataType*, std::string>> inputParams;
	if(myEntrypointDefinition.returnType.isStruct)
	{
		auto* hlslDef = (HLSLDefiner::LangDefinition*)parentScript->GetLanguageDefinition();

		for (const auto& member : hlslDef->structs[myEntrypointDefinition.returnType.typeName].members)
		{
			inputParams.push_back({ &member.type, member.name });
		}
	}
	else
	{
		inputParams.push_back({ &myEntrypointDefinition.returnType, "OUTPUT" });
	}

	return inputParams;
}

void KE::CodeExitPointNode::Init()
{
	AddInputPin(ValueType::FlowValue, PinType::Flow, "");

	const auto& params = GetParameters();

	for (const auto& param : params)
	{
		AddInputPin(ValueType::CodeValue, PinType::Value, param.second.c_str());
		codePinData.push_back({ CodePinType::Value, { *param.first } });
	}

	for (int i = 0; i < params.size(); i++)
	{
		inputPins[i + 1].codeData = &codePinData[i];
	}
}

KE::CommandList KE::CodeExitPointNode::GetCommands(const LanguageDefinitionNew& aLanguage)
{
	CommandList commands;

	const auto& params = GetParameters();
	for (int i = 0; i < params.size(); i++)
	{
		AssignValueCommand* assignValueCommand = commands.Add<AssignValueCommand>();
		assignValueCommand->assignFrom = inputPins[i+1].ID;
		assignValueCommand->assignTo = std::format("{}.{}", "OUTPUT", params[i].second);
	}

	return commands;
}
#pragma endregion
