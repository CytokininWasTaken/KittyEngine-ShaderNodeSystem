#include "stdafx.h"
#include "NodeDatabase.h"

#include "ScriptExecution.h"
#include "Script.h"

#include <Editor/Source/EditorWindows/NodeEditor.h>

#include "Collision/Collider.h"
#include "Collision/Layers.h"
#include "ComponentSystem/GameObject.h"
#include "ComponentSystem/Components/Component.h"
#include "Engine/Source/Math/KittyMath.h"
#include <External/Include/nlohmann/json.hpp>
#include <Engine/Source/AI/BehaviourTree/BehaviourTreeBuilder.h>

#include "CodeNode.h"


//project!

#include "HLSLDefiner.h"
#include "ComponentSystem/Components/Collider/BoxColliderComponent.h"
#include "Editor/Source/ImGui/ImGuiHandler.h"
#include "ImNodes/ImNodes.h"

//

#define DefineNode(name, category, description) \
	const char* GetName() const override { return name; } \
	NodeCategory GetCategory() const override { return category; } \
	const char* GetDescription() const override { return description; }\

KE::NodeTypeDatabase::NodeTypeDatabase()
{
}

KE::NodeTypeDatabase::~NodeTypeDatabase()
{
	for (auto& language : myLanguageDefinitions)
	{
		delete language.second;
	}
}

void KE::NodeTypeDatabase::RegisterLanguage(const std::string& aLanguageFilePath)
{

	auto languageDef = LanguageDefinerNew<HLSLDefiner>::Define(aLanguageFilePath.c_str());
	myLanguageDefinitions["hlsl"] = new HLSLDefiner::LangDefinition(languageDef);

	for (const auto& function : languageDef.functions)
	{
		RegisterNode(
			function.second.name.c_str(),
			NodeCategory::CodeLogic,
			"",
			[function, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeFunctionNode* node = new CodeFunctionNode();
				/*node->SetVariantIndex(0);*/
				node->SetVariantCount(static_cast<unsigned int>(function.second.variants.size()));
				node->AddData(function.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Functions"
		);
	}

	for (const auto& dataStruct : languageDef.structs)
	{
		RegisterNode(
			dataStruct.second.name.c_str(),
			NodeCategory::CodeLogic,
			"",
			[dataStruct, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeStructNode* node = new CodeStructNode();
				node->AddData(dataStruct.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Structs"
		);
	}

	for (const auto& buffer : languageDef.buffers)
	{
		RegisterNode(
			buffer.second.name.c_str(),
			NodeCategory::CodeLogic,
			"",
			[buffer, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeBufferNode* node = new CodeBufferNode();
				node->AddData(buffer.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Buffers"
		);
	}

	for (const auto& texture : languageDef.textures)
	{
		RegisterNode(
			texture.second.name.c_str(),
			NodeCategory::CodeLogic,
			"",
			[texture, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeTextureNode* node = new CodeTextureNode();
				node->AddData(texture.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Textures"
		);
	}

	for (const auto& entryPoint : languageDef.entrypoints)
	{
		std::string entryPointName = entryPoint.second.name;
		std::string inName = "Enter " + entryPointName;
		std::string outName = "Exit " + entryPointName;


		RegisterNode(
			inName.c_str(),
			NodeCategory::CodeLogic,
			"",
			[entryPoint, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeEntryPointNode* node = new CodeEntryPointNode();
				node->AddData(entryPoint.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Entrypoints"
		);

		RegisterNode(
			outName.c_str(),
			NodeCategory::CodeLogic,
			"",
			[entryPoint, languageDef](Script* aScript) -> ScriptNode*
			{
				CodeExitPointNode* node = new CodeExitPointNode();
				node->AddData(entryPoint.second);
				node->AssignScript(aScript);
				return node;
			},
			"HLSL/Exitpoints"
		);
	}

}

void KE::NodeTypeDatabase::CalculateNodeCategoryStack(const std::string& aNodeName, const char* aCategoryStack)
{
	std::string categoryStack = aCategoryStack;
	std::string category = "";
	for (int i = 0; i < categoryStack.size(); i++)
	{
		if (categoryStack[i] == '/')
		{
			if (category.size() > 0)
			{
				nodeCategoryStacks[aNodeName].names.push_back(category);
				category = "";
			}
		}
		else
		{
			category += categoryStack[i];
		}
	}
	if (category.size() > 0)
	{
		nodeCategoryStacks[aNodeName].names.push_back(category);
	}
}

void KE::NodeTypeDatabase::ProcessNodeData(const std::string& aNodeName)
{
	Script tempScript;
	tempScript.SetLanguageDefinition(myLanguageDefinitions["hlsl"]);

	KE::ScriptNode* node = nodeTypes[aNodeName].createFunction(&tempScript);
	node->Init();

	auto* variant = dynamic_cast<IVariant*>(node);
	unsigned int iterCount = variant ? variant->GetVariantCount() : 1;

	for (unsigned int variantIter = 0; variantIter < iterCount; variantIter++)
	{
		if (variant) { variant->SetVariantIndex(variantIter); }

		for (int i = 0; i < node->GetInputPins().size(); i++)
		{
			KE::Pin* pin = &node->GetInputPins()[i];

			NodePinDatabaseValue v{ i, "", "", &nodeTypes[aNodeName], variantIter };
			strcpy_s(v.pinName, pin->name);
			strcpy_s(v.nodeName, aNodeName.c_str());
			if (pin->codeData)
			{
				nodePinCodeInDatabase[((CodePin*)pin->codeData)->value.type.typeName].push_back(v);
			}
			else
			{
				nodeInPinDatabase[pin->value.type].push_back(v);
			}
		}

		for (int i = 0; i < node->GetOutputPins().size(); i++)
		{
			KE::Pin* pin = &node->GetOutputPins()[i];

			NodePinDatabaseValue v{ i, "", "", &nodeTypes[aNodeName], variantIter };
			strcpy_s(v.pinName, pin->name);
			strcpy_s(v.nodeName, aNodeName.c_str());

			if (pin->codeData)
			{
				nodePinCodeOutDatabase[((CodePin*)pin->codeData)->value.type.typeName].push_back(v);
			}
			else
			{
				nodeOutPinDatabase[pin->value.type].push_back(v);
			}
		}
	}

	delete node;
}

void KE::NodeTypeDatabase::RegisterNode(const char* aNodeName, NodeCategory aCategory, const char* aDescription, const NodeCreateFunction& aCreateFunction, const char* aCategoryStack)
{
	nodeTypes[aNodeName] = { aNodeName, aCategory, aDescription, aCreateFunction };
	ProcessNodeData(aNodeName);
	CalculateNodeCategoryStack(aNodeName, aCategoryStack);
	nodeNameMap[aNodeName] = aNodeName;
}
