#pragma once
#include <d3d11.h>
#include <unordered_map>

#include "CodeNode.h"
#include "Graphics/Shader.h"

namespace KE
{
	struct Material;
	struct ScriptMemberID;
	class Script;

	struct ParsedConnection
	{
		ScriptMemberID from;
		ScriptMemberID to;
	};

	struct ValueNodeTreeEntry
	{
		ScriptMemberID nodeID;
		std::vector<ValueNodeTreeEntry> parents;
		std::vector<ValueNodeTreeEntry> children;
		std::vector<ScriptMemberID> nonValueChildren;
		std::vector<ScriptMemberID> nonValueParents;
		bool isRoot = false; // we can have multiple root nodes
		bool isLeaf = false;
		bool isUsed = false;
	};

	struct ValueNodeTree
	{
		std::vector<int> rootIndices;
		std::vector<ValueNodeTreeEntry> entries;
	};

	struct DependencyMapping
	{
		std::vector<ScriptMemberID> dependencies;
	};

	struct GeneratedVariable
	{
		std::string name;
		std::string type;
		std::string value;

		bool isConstant = false;
	};



	struct ParsingData
	{
		Script* script;
		const LanguageDefinitionNew* languageDefinition;

		std::unordered_map<ScriptMemberID, ScriptNode*, ScriptMemberID>& scriptNodes;
		std::unordered_map<ScriptMemberID, std::vector<PinConnection>, ScriptMemberID>& scriptConnections;

		std::vector<ParsedConnection> nodeFlow;
		std::vector<ValueNodeTree> nodeTrees;
		std::unordered_map<ScriptMemberID, int, ScriptMemberID> idToTreeIndexMap;
		std::unordered_map<ScriptMemberID, DependencyMapping, ScriptMemberID> dependencyMap;
		std::unordered_map<ScriptMemberID, std::vector<ScriptMemberID>, ScriptMemberID> dependents;

		std::unordered_map<ScriptMemberID, std::string, ScriptMemberID> parsedNodeMap;
		std::unordered_map<ScriptMemberID, std::vector<GeneratedVariable>, ScriptMemberID> generatedVariables;

		//new functionality
		std::unordered_map<ScriptMemberID, CodeVariable, ScriptMemberID> tempNewVariables;
		std::unordered_map<ScriptMemberID, CodeVariable, ScriptMemberID> newVariables;

		std::vector<ScriptMemberID> parsingOrder;

		std::unordered_map<ScriptMemberID, std::string, ScriptMemberID> generatedCode;

		std::unordered_map<ScriptMemberID, std::vector<CodeScope*>, ScriptMemberID> evaluatedScopeData;

		std::vector<ScriptMemberID> multiplyScopedNodes;
		std::vector<ScriptMemberID> unscopedNodes;
		std::vector<ScriptMemberID> globallyScopedNodes;

		std::unordered_map<ScriptMemberID, std::string, ScriptMemberID> outputRenderNodes;

		std::vector<CodeScope> codeScopes;
		size_t currentScopeDepth = 0;
		
		ParsingData(Script* aScript);
	};


	struct ParsingOutput
	{
		ParsingData parsingData;
		std::string code;

		ParsingOutput(Script* aScript) : parsingData(aScript) {}
	};

	class CodeScriptParser
	{
	private:
		static Pin* GetUsedPin(ParsingData& aData, ScriptMemberID aPinID);
		static PinValue GetPinValue(ParsingData& aData, ScriptMemberID aPinID);

		static std::string EvaluateVariable(ParsingData& aData, const std::string& variableName, ScriptNode* aNode);
		static std::string ReplaceCodeVariableNames(ParsingData& aData, const std::string& nodeCode, ScriptNode* aNode);

		static void EvaluateScope(ParsingData& aData, ScriptMemberID aNodeID);
		static bool IsInScope(ParsingData& aData, ScriptMemberID aNodeID, const ScopePinList& aScopeMemberList, std::unordered_map<ScriptMemberID, bool, ScriptMemberID>& aWorkingData);
		static std::string WriteScope(ParsingData& aData, CodeScope* aScope);
		static void CreateScopeStack(ParsingData& aData, std::vector<CodeScope*>& aOutScopeStack);
		static void AddScopeIndenting(ParsingData& aData, std::string& aCode);

		static void FindDependentScopes(ParsingData& aData, ScriptMemberID aNodeID);
		static void CorrectUnscopedNodes(ParsingData& aData);

		static void DependencyHelper(ParsingData& aData, ScriptMemberID aNodeID, std::unordered_map<ScriptMemberID, bool, ScriptMemberID>& aWorkingData);
		static void EvaluateDependencies(ParsingData& aData);
		

		static void AppendParsingOrder(ParsingData& aData, ScriptMemberID aNodeID);
		static void ParsingOrderHelper(ParsingData& aData, ScriptMemberID aNodeID);
		static void GenerateParsingOrder(ParsingData& aData);

		static std::string ParseCodeNode(ParsingData& aData, ScriptNode* aNodeToParse);
		static std::string GenerateCode(ParsingData& aData);

		static bool IsLinkNode(const ParsingData& aData, ScriptMemberID aNodeID);

	public:
		static void GeneratePreviewShaders(Script* aScript, const ParsingOutput& aData, Graphics* aGraphics, std::unordered_map<ScriptMemberID, PixelShader*, ScriptMemberID>& generatedShaderMap);
		static void RenderPreviewImages(Script* aScript, const ParsingOutput& aData, std::unordered_map<KE::ScriptMemberID, int, KE::ScriptMemberID>&
		                                  aNodeTextureLinkageMap, std::array<ID3D11ShaderResourceView*, 16>& aOutShaderTextures, Graphics* aGraphics, Material* aMaterial = nullptr);
		static bool Parse(Script* aScript, ParsingOutput*& anOutput);

		static void GetOutputRenderCode(const ParsingOutput& aData, std::vector<std::pair<ScriptMemberID, std::string>>& aOutputCodeList);

		static ScriptMemberID GetLineAuthor(ParsingData& aData, const std::string& aLine);
		static std::pair<size_t, size_t> GetCodeAreaFromAuthor(const ParsingData& aData, ScriptMemberID anAuthor, const std::string& aCode);


	};


}
