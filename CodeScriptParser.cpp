#include "stdafx.h"
#include "Node.h"
#include "Script.h"
#include "CodeScriptParser.h"

#include "LanguageData.h"
#include "Graphics/Graphics.h"
#include "Graphics/ModelData.h"
#include "nlohmann/json.hpp"

KE::ParsingData::ParsingData(Script* aScript) : script(aScript), scriptNodes(aScript->GetNodes()), scriptConnections(aScript->GetConnections())
{
}

KE::Pin* KE::CodeScriptParser::GetUsedPin(ParsingData& aData, ScriptMemberID aPinID)
{
	KE::Pin* workingPin = aData.script->GetPin(aPinID);
	if (!workingPin) { return nullptr; }
	const bool isOutputPin = aData.script->PinIsOutput(workingPin);

	if (IsLinkNode(aData, aPinID.GetNodeID()) && isOutputPin)
	{
		const auto linkConnections = aData.scriptConnections[aData.scriptNodes.at(aPinID.GetNodeID())->GetInputPins()[0].ID];
		if (linkConnections.empty()) { return workingPin; }


		return GetUsedPin(aData, aData.scriptNodes.at(aPinID.GetNodeID())->GetInputPins()[0].ID);
	}

	if (isOutputPin || aData.scriptConnections[aPinID].empty())
	{
		return workingPin;
	}

	const auto to = aData.scriptConnections[aPinID][0].to;

	if (!isOutputPin)
	{
		//just use the 0th connection for now
		return GetUsedPin(aData, to);
	}

	return workingPin;
}

KE::PinValue KE::CodeScriptParser::GetPinValue(ParsingData& aData, ScriptMemberID aPinID)
{
	return GetUsedPin(aData, aPinID)->value;
}

std::string KE::CodeScriptParser::EvaluateVariable(ParsingData& aData, const std::string& variableName, ScriptNode* aNode)
{
	std::string out = "";

	ScriptMemberID referencedID = std::stoi(variableName);
	Pin* pin = GetUsedPin(aData, referencedID);

	if (!pin) { return out; }
	if (aData.newVariables.contains(pin->ID))
	{
		const auto& variable = aData.newVariables[pin->ID];
		out = variable.parsedName;
	}
	else
	{
		out = std::to_string(GetPinValue(aData, pin->ID).value.Float);
		
	}
	
	return out;
}

std::string KE::CodeScriptParser::ReplaceCodeVariableNames(ParsingData& aData, const std::string& nodeCode, ScriptNode* aNode)
{
	std::string out;
	constexpr char identifier = '$';

	size_t off = 0;
	while (nodeCode.find(identifier, off) != std::string::npos)
	{
		const size_t firstPos = nodeCode.find(identifier, off);
		const size_t nextPos = nodeCode.find(identifier, firstPos + 1);

		std::string variableName = nodeCode.substr(firstPos + 1, nextPos - firstPos - 1);

		const std::string replacedVariableName = EvaluateVariable(aData, variableName, aNode);

		out += nodeCode.substr(off, firstPos - off);
		out += replacedVariableName;

		off = nextPos + 1;
	}
	out += nodeCode.substr(off, nodeCode.size() - off);

	return out;
}

void KE::CodeScriptParser::EvaluateScope(ParsingData& aData, ScriptMemberID aNodeID)
{
	size_t workingDepth = 0;
	CodeScope* deepestScope = nullptr;
	

	for (auto& scope : aData.codeScopes)
	{
		std::unordered_map<ScriptMemberID, bool, ScriptMemberID> workingData;

		const size_t scopeDepth = scope.GetScopeDepth();
		if (deepestScope && workingDepth >= scopeDepth) { continue; }
		if (IsInScope(aData, aNodeID, scope.scopePins, workingData))
		{
			workingDepth = scope.GetScopeDepth();
			deepestScope = &scope;
		}
	}

	if (deepestScope)
	{
		deepestScope->scopeMembers.push_back(aNodeID);
		aData.evaluatedScopeData[aNodeID].push_back(deepestScope);
	}
	else
	{
		aData.unscopedNodes.push_back(aNodeID);
	}
}

bool KE::CodeScriptParser::IsInScope(KE::ParsingData& aData, KE::ScriptMemberID aNodeID, const ScopePinList& aScopeMemberList, std::unordered_map<KE::ScriptMemberID, bool, KE::ScriptMemberID>& aWorkingData)
{
	if (aNodeID.GetNodeID() == aScopeMemberList.pins.at(0).GetNodeID()) { return false; } //we've reached the origin of the scope
	
	if (aWorkingData.contains(aNodeID))
	{
		return false; //we've already evaluated this node, so it should have returned true already if it was in scope.
	}
	if (aData.scriptNodes[aNodeID] == nullptr)
	{
		return false;
	}

	//flag ID to prevent infinite recursion
	aWorkingData[aNodeID] = true;
	
	for (auto& pin : aData.scriptNodes[aNodeID]->GetInputPins())
	{
		if (aData.scriptConnections.contains(pin.ID))
		{
			for (auto& connection : aData.scriptConnections[pin.ID])
			{
				if (aScopeMemberList.Contains(connection.to)) // this pin has a direct connection to the scope origin
				{
					return true;
				}

				if (IsLinkNode(aData, connection.to.GetNodeID()))
				{
					auto linkConnections = aData.scriptConnections[aData.scriptNodes.at(connection.to.GetNodeID())->GetInputPins()[0].ID];
					if (linkConnections.empty()) { continue; }

					auto linkToNode = linkConnections[0].to.GetNodeID();

					if (IsInScope(aData, connection.to.GetNodeID(), aScopeMemberList, aWorkingData))
					{
						return true;
					}

					continue;
				}

				if (IsInScope(aData, connection.to.GetNodeID(), aScopeMemberList, aWorkingData))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

std::string KE::CodeScriptParser::WriteScope(ParsingData& aData, CodeScope* aScope)
{
	if (aScope->hasClosed) { return ""; }
	aData.currentScopeDepth += 1;
	std::string out;


	out += aScope->Open();

	std::string scopeCode;
	for (ScriptMemberID node : aData.parsingOrder)
	{
		if (std::ranges::find(aScope->scopeMembers, node) == aScope->scopeMembers.end()) { continue; }
		scopeCode += aData.generatedCode[node];

	}
	AddScopeIndenting(aData, scopeCode);
	out += scopeCode;

	for (CodeScope* childScope : aScope->childScopes)
	{
		out += WriteScope(aData, childScope);
	}
	out += aScope->Close();

	aData.currentScopeDepth -= 1;
	AddScopeIndenting(aData, out);
	return out;
}

void KE::CodeScriptParser::CreateScopeStack(ParsingData& aData, std::vector<CodeScope*>& aOutScopeStack)
{
	for (auto& scope : aData.codeScopes)
	{
		aOutScopeStack.push_back(&scope);
	}
}

void KE::CodeScriptParser::AddScopeIndenting(ParsingData& aData, std::string& aCode)
{
	std::string spacing;
	constexpr size_t spacingSpaceCount = 2;
	for (size_t i = 0; i < aData.currentScopeDepth * spacingSpaceCount; i++) { spacing += " "; }

	std::istringstream iss(aCode);

	std::vector<std::string> lines;

	std::string indentedString;
	for (std::string line; std::getline(iss, line); )
	{
		lines.push_back(line);
		indentedString += spacing + line + "\n";
	}
	
	aCode = indentedString;
}

void KE::CodeScriptParser::FindDependentScopes(ParsingData& aData, ScriptMemberID aNodeID)
{
	const auto& node = aData.scriptNodes[aNodeID];
	for (auto& outputPin : node->GetOutputPins())
	{
		for (auto& connection : aData.scriptConnections[outputPin.ID])
		{
			const std::vector<CodeScope*>& connectedNodeScopes = aData.evaluatedScopeData[connection.to.GetNodeID()];
			if (connectedNodeScopes.empty())
			{
				FindDependentScopes(aData, connection.to.GetNodeID());
				continue;
			}
			for (auto& scope : connectedNodeScopes)
			{
				auto& nodeScopes = aData.evaluatedScopeData[aNodeID];

				if (nodeScopes.empty() || std::ranges::find(nodeScopes, scope) == nodeScopes.end())
				{
					nodeScopes.push_back(scope);
				}
			}
		}
	}
}

void KE::CodeScriptParser::CorrectUnscopedNodes(ParsingData& aData)
{
	//some nodes (those that only in- and output values) don't have a scope yet. We need to find a place for them.
	for (ScriptMemberID nodeID : aData.unscopedNodes)
	{
		FindDependentScopes(aData, nodeID);

		if (aData.evaluatedScopeData[nodeID].size() == 1)
		{
			CodeScope* scope = aData.evaluatedScopeData[nodeID].back();
			scope->scopeMembers.push_back(nodeID);
			continue;
		}
		
		bool matchingParentScopes = true;
		CodeScope* sharedParentScope = aData.evaluatedScopeData[nodeID].empty() ? &aData.codeScopes[0] : aData.evaluatedScopeData[nodeID][0]->parentScope;

		for (auto* scope : aData.evaluatedScopeData[nodeID])
		{
			if (scope->parentScope != sharedParentScope) { matchingParentScopes = false; }
		}

		
		if(matchingParentScopes)
		{
			sharedParentScope->scopeMembers.push_back(nodeID);
			aData.evaluatedScopeData[nodeID] = { sharedParentScope };
			continue;
		}
	
		for (auto* scope : aData.evaluatedScopeData[nodeID])
		{
			scope->scopeMembers.push_back(nodeID);
		}
	}
}

void KE::CodeScriptParser::DependencyHelper(ParsingData& aData, ScriptMemberID aNodeID, std::unordered_map<ScriptMemberID, bool, ScriptMemberID>& aWorkingData)
{
	if (aWorkingData[aNodeID]) { return; }
	if (!aData.scriptNodes[aNodeID]) { return; }

	for (const auto& input : aData.scriptNodes[aNodeID]->GetInputPins())
	{
		for (const auto& connection : aData.scriptConnections[input.ID])
		{
			auto toNode = connection.to.GetNodeID();

			if (IsLinkNode(aData, toNode))
			{
				auto linkConnections = aData.scriptConnections[aData.scriptNodes.at(toNode)->GetInputPins()[0].ID];
				if (linkConnections.empty()) { continue; }

				auto linkToNode = linkConnections[0].to.GetNodeID();

				DependencyHelper(aData, linkToNode, aWorkingData);

				if (std::ranges::find(aData.dependencyMap[aNodeID].dependencies, linkToNode) == aData.dependencyMap[aNodeID].dependencies.end())
				{
					aData.dependencyMap[aNodeID].dependencies.push_back(linkToNode);
				}

				if (std::ranges::find(aData.dependents[linkToNode].begin(), aData.dependents[linkToNode].end(), aNodeID) == aData.dependents[linkToNode].end())
				{
					aData.dependents[linkToNode].push_back(aNodeID);
				}

				continue;
			}

			DependencyHelper(aData, toNode, aWorkingData);

			if (std::ranges::find(aData.dependencyMap[aNodeID].dependencies, toNode) == aData.dependencyMap[aNodeID].dependencies.end())
			{
				aData.dependencyMap[aNodeID].dependencies.push_back(toNode);
			}

			if (std::ranges::find(aData.dependents[toNode].begin(), aData.dependents[toNode].end(), aNodeID) == aData.dependents[toNode].end())
			{
				aData.dependents[toNode].push_back(aNodeID);
			}
		}
	}
}

void KE::CodeScriptParser::EvaluateDependencies(ParsingData& aData)
{
	std::unordered_map<ScriptMemberID, bool, ScriptMemberID> evaluated;

	for (const auto& node : aData.scriptNodes)
	{
		DependencyHelper(aData, node.first, evaluated);
	}

}

std::string KE::CodeScriptParser::ParseCodeNode(ParsingData& aData, ScriptNode* aNodeToParse)
{
	if (aNodeToParse == nullptr) { return "not a node"; }

	std::string out;
	KE::CodeScriptNode* codeNode = dynamic_cast<KE::CodeScriptNode*>(aNodeToParse);
	if (codeNode == nullptr) { return std::format("{} is not a code node!\n", aNodeToParse->ID.idParts.nodeID); }

	EvaluateScope(aData, aNodeToParse->ID);

	auto nodeCommandList = codeNode->GetCommands(*aData.languageDefinition);
	for (const auto& command : nodeCommandList.GetCommands())
	{
		std::string commandOut;

		const auto parsedCommand = command->Parse();

		switch (parsedCommand.type)
		{
		case ParseResult::ResultType::CreateScope:
		{
			aData.codeScopes.push_back(*parsedCommand.scope);
			auto* newScope = &aData.codeScopes.back();

			if (!aData.evaluatedScopeData[aNodeToParse->ID].empty())
			{
				const auto& nodeScope = aData.evaluatedScopeData[aNodeToParse->ID][0];
				newScope->parentScope = nodeScope;
				nodeScope->childScopes.push_back(newScope);
				
			}
			if (newScope->parentScope == nullptr) { newScope->parentScope = &aData.codeScopes[0]; }


			break;
		}
		case ParseResult::ResultType::GenerateVariable:
		{
			const auto* variable = parsedCommand.variable;

			aData.newVariables[variable->originID] = *variable;
			switch(variable->level)
			{
			case CodeVariableLevel::Register:
			{
				break;
			}
			case CodeVariableLevel::Declare:
			{
				commandOut += std::format(
					"{} {} {};\n",
					parsedCommand.variable->parsedMutability,
					parsedCommand.variable->parsedType,
					parsedCommand.variable->parsedName
				);
				break;
			}
			case CodeVariableLevel::Define:
			{
				commandOut += std::format(
					"{} {} {} {};\n",
					parsedCommand.variable->parsedMutability,
					parsedCommand.variable->parsedType,
					parsedCommand.variable->parsedName,
					parsedCommand.variable->parsedAssignment
				);

				break;
			}
			default:break;
			}

			commandOut = ReplaceCodeVariableNames(aData, commandOut, aNodeToParse);

			if (variable->level == CodeVariableLevel::Define && parsedCommand.variable->parsedType == "float4")
			{
				aData.outputRenderNodes.insert({ variable->originID, parsedCommand.variable->parsedName });
			}

			break;
		}
		case ParseResult::ResultType::ReturnValue:
		{

			commandOut += std::format("return {};\n", parsedCommand.returnValue->parsedReturn);
			commandOut = ReplaceCodeVariableNames(aData, commandOut, aNodeToParse);
			break;
		}
		case ParseResult::ResultType::AssignValue:
		{

			commandOut += std::format("{} = {};\n", parsedCommand.assign->assignTo, parsedCommand.assign->parsedAssignment);
			commandOut = ReplaceCodeVariableNames(aData, commandOut, aNodeToParse);
		}
		default: break;
		}
		out += commandOut;
	}

	return out;
}

std::string KE::CodeScriptParser::GenerateCode(ParsingData& aData)
{
	std::string out;


	for (const auto& node : aData.parsingOrder)
	{
		aData.generatedCode[node.GetNodeID()] = ParseCodeNode(aData, aData.scriptNodes[node.GetNodeID()]);
	}

	CorrectUnscopedNodes(aData);

	for (auto& scope : aData.codeScopes)
	{
		out += WriteScope(aData, &scope);
	}

	return out;
}

bool KE::CodeScriptParser::IsLinkNode(const ParsingData& aData, ScriptMemberID aNodeID)
{
	if (!aData.scriptNodes.contains(aNodeID)) { return false; }
	if (!aData.scriptNodes[aNodeID]) { return false; }
	const char* name = aData.scriptNodes.at(aNodeID)->GetName();

	return (strcmp(name, "Value Link") == 0 || strcmp(name, "Flow Link") == 0);
}

void KE::CodeScriptParser::GeneratePreviewShaders(
	Script* aScript,
	const ParsingOutput& aData,
	Graphics* aGraphics,
	std::unordered_map<ScriptMemberID, PixelShader*, ScriptMemberID>& generatedShaderMap
)
{
	std::vector<std::pair<KE::ScriptMemberID, std::string>> outputCodeList;
	GetOutputRenderCode(aData, outputCodeList);

	for (int i = 0; i < outputCodeList.size(); i++)
	{
		generatedShaderMap[outputCodeList[i].first] = aGraphics->GetShaderLoader().CreatePixelShaderFromParsedScript(
			std::format("shaderPreviewScript_{}_PS", outputCodeList[i].first.idParts.nodeID),
			outputCodeList[i].second, "mainPreview"
		);
	}
}

void KE::CodeScriptParser::RenderPreviewImages(
	Script* aScript,
	const ParsingOutput& aData,
	std::unordered_map<KE::ScriptMemberID, int, KE::ScriptMemberID>& aNodeTextureLinkageMap,
	std::array<ID3D11ShaderResourceView*, 16>& aOutShaderTextures,
	Graphics* aGraphics,
	KE::Material* aPreviewRenderMaterial
)
{
	ModelData modelData;
	Transform t;

	modelData.myMeshList = &aGraphics->GetModelLoader().Load("Data/EngineAssets/SmoothSphere.fbx");

	modelData.myRenderResources.emplace_back();

	if (aPreviewRenderMaterial)
	{
		modelData.myRenderResources[0].myMaterial = aPreviewRenderMaterial;
	}
	else
	{
		modelData.myRenderResources[0].myMaterial = aGraphics->GetTextureLoader().GetDefaultMaterial();
	}

	if (aData.parsingData.codeScopes.size() > 1 && aData.parsingData.codeScopes[1].scopeStart.find("VFX") != std::string::npos)
	{
		modelData.myRenderResources[0].myVertexShader = aGraphics->GetShaderLoader().GetVertexShader(SHADER_LOAD_PATH "Model_VFX_VS.cso");
	}
	else
	{
		modelData.myRenderResources[0].myVertexShader = aGraphics->GetShaderLoader().GetVertexShader(SHADER_LOAD_PATH "Model_Deferred_VS.cso");
	}

	modelData.myTransform = &t.GetMatrix();
	t.SetScale({ 0.65f,0.65f,0.65f });
	
	//this gives us a 16x16 grid of 256x256 previews, which should be enough for now (that's 256 previews)
	constexpr int previewAtlasSize = 4096;
	constexpr int previewSize      = 256;

	Camera renderCamera;
	renderCamera.SetPerspective(previewSize, previewSize, 90.0f * KE::DegToRadImmediate, 0.01f, 10.f);
	renderCamera.transform.SetPosition({ 0.0f, 0.0f, -1.0f });

	BasicRenderInput in;
	in.viewMatrix = renderCamera.GetViewMatrix();
	in.projectionMatrix = renderCamera.GetProjectionMatrix();

	std::vector<std::pair<KE::ScriptMemberID, std::string>> outputCodeList;
	GetOutputRenderCode(aData, outputCodeList);

	auto* rt = aGraphics->GetRenderTarget(9); 
	rt->Resize(previewAtlasSize, previewAtlasSize);
	float clearColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	rt->Clear(clearColour);

	rt->MakeActive(true);

	aGraphics->SetView(in.viewMatrix);
	aGraphics->SetProjection(in.projectionMatrix);
	aGraphics->SetCommonBuffer(renderCamera);


	for (int i = 0; i < outputCodeList.size(); i++)
	{
		modelData.myRenderResources[0].myPixelShader = aGraphics->GetShaderLoader().GetPixelShader(
			std::format("shaderPreviewScript_{}_PS", outputCodeList[i].first.idParts.nodeID)
		);
		if (modelData.myRenderResources[0].myPixelShader == aGraphics->GetShaderLoader().GetDefaultPixelShader()) {continue; }

		Timer renderModelTimer;
		const float viewportX = (float)(i % 16) * (float)previewSize;
		const float viewportY = (float)(i / 16) * (float)previewSize;

		aGraphics->SetViewport(previewSize, previewSize, viewportX, viewportY);

		aGraphics->GetDefaultRenderer()->RenderModel(in, modelData);

		aNodeTextureLinkageMap[outputCodeList[i].first.GetNodeID()] = i;
	}

	aOutShaderTextures[0] = modelData.myRenderResources[0].myMaterial->myTextures[0]->myShaderResourceView.Get();
	aOutShaderTextures[1] = modelData.myRenderResources[0].myMaterial->myTextures[1]->myShaderResourceView.Get();
	aOutShaderTextures[2] = modelData.myRenderResources[0].myMaterial->myTextures[2]->myShaderResourceView.Get();
	aOutShaderTextures[3] = modelData.myRenderResources[0].myMaterial->myTextures[3]->myShaderResourceView.Get();
}


bool KE::CodeScriptParser::Parse(Script* aScript, ParsingOutput*& anOutput)
{
	std::string out;

	anOutput = new ParsingOutput{aScript};
	anOutput->parsingData.languageDefinition = aScript->GetLanguageDefinition();


	anOutput->parsingData.codeScopes.reserve(1024);
	anOutput->parsingData.codeScopes.push_back(CodeScope(INT_MAX, {}, {}, "", "", nullptr));
	anOutput->parsingData.codeScopes.back().scopePins.pins.push_back(INT_MAX);


	const auto& nodes = aScript->GetNodes();
	ScriptMemberID entryID = INT_MAX;
	for (const auto& node : nodes)
	{
		if (node.second->GetCategory() == NodeCategory::CodeEntryPoint)
		{
			entryID = node.first;
			break;
		}
	}
	if (entryID.combinedID == INT_MAX) { return false; }

	EvaluateDependencies(anOutput->parsingData);

	GenerateParsingOrder(anOutput->parsingData);

	out += GenerateCode(anOutput->parsingData);


	anOutput->code = out;

	return true;
}

void KE::CodeScriptParser::GetOutputRenderCode(const ParsingOutput& aData, std::vector < std::pair<ScriptMemberID, std::string >> & aOutputCodeList)
{

	for (const auto& node : aData.parsingData.outputRenderNodes)
	{
		std::string out;

		if (aData.parsingData.codeScopes[1].scopeStart.find("VFX") != std::string::npos)
		{
			out += "float4 mainPreview(VFXPixelInput aInput) : SV_TARGET0\n{\n";
		}
		else
		{
			out += "float4 mainPreview(PixelInput aInput) : SV_TARGET0\n{\n";
		}

		for (const auto& parseID : aData.parsingData.parsingOrder)
		{
			std::string nodeCode = aData.parsingData.generatedCode.at(parseID);
			if (nodeCode.find("not a") != std::string::npos) { continue; }
			if (nodeCode.find("OUTPUT") != std::string::npos) { continue; }
			out += nodeCode;

			if (parseID == node.first.GetNodeID())
			{

				const size_t varNameStart = nodeCode.find_first_of("float4") + strlen("float4");
				const size_t varNameEnd = nodeCode.find_first_of(" ", varNameStart+1);
				std::string varName = nodeCode.substr(varNameStart+1, varNameEnd-varNameStart);

				out += std::format("\n\treturn {};\n", varName);
				break;
			}
		}
		out += "}";

		aOutputCodeList.push_back({ node.first, out });
	}
}

void KE::CodeScriptParser::AppendParsingOrder(ParsingData& aData, ScriptMemberID aNodeID)
{
	if (std::ranges::find(aData.parsingOrder,aNodeID) == aData.parsingOrder.end())
	{
		aData.parsingOrder.push_back(aNodeID);
	}
}

void KE::CodeScriptParser::ParsingOrderHelper(ParsingData& aData, ScriptMemberID aNodeID)
{
	for (const auto& dependency : aData.dependencyMap[aNodeID].dependencies)
	{
		ParsingOrderHelper(aData, dependency.GetNodeID());
	}
	AppendParsingOrder(aData, aNodeID);
}

void KE::CodeScriptParser::GenerateParsingOrder(ParsingData& aData)
{
	for (const auto& node : aData.scriptNodes)
	{
		ParsingOrderHelper(aData, node.first);
	}
}

KE::ScriptMemberID KE::CodeScriptParser::GetLineAuthor(ParsingData& aData, const std::string& aLine)
{
	ScriptMemberID out;
	out.combinedID = INT_MIN;

	for (const auto& node : aData.scriptNodes)
	{
		if (aData.parsedNodeMap[node.first.GetNodeID()].find(aLine) != std::string::npos)
		{
			out = node.first;
			break;
		}
	}

	return out;
}

std::pair<size_t, size_t> KE::CodeScriptParser::GetCodeAreaFromAuthor(const ParsingData& aData, ScriptMemberID anAuthor, const std::string& aCode)
{
	std::pair<size_t, size_t> markers = {0,0};
	size_t matchPos = std::string::npos;

	for (const auto& parsed : aData.generatedCode)
	{
		if (parsed.first == anAuthor)
		{
			matchPos = aCode.find(parsed.second);
			if (matchPos != std::string::npos)
			{
				markers.first = matchPos;
				markers.second = matchPos + parsed.second.size();
				return markers;
			}
		}
	}

	return markers;
}