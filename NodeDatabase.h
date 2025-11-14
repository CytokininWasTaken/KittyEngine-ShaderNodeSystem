#pragma once
#include <functional>

#include "LanguageData.h"
#include "Node.h"


namespace KE
{
	typedef std::function<ScriptNode* (Script* aScript)> NodeCreateFunction;

	class ScriptNode;
	class NodeTypeDatabase
	{
	public:
		struct NodeTypeData
		{
			std::string name;
			NodeCategory category;
			std::string description;
			NodeCreateFunction createFunction;
		};

		struct NodeCategoryStack
		{
			std::vector<std::string> names;
		};

		struct NodePinDatabaseValue
		{
			int index = -1;
			char pinName[PIN_STRING_MAX]  = "";
			char nodeName[PIN_STRING_MAX] = "";
			NodeTypeData* aNodeType = nullptr;
			unsigned int variantIndex;
		};

		std::unordered_map<std::string, LanguageDefinitionNew*> myLanguageDefinitions; //todo: this don't live here

		std::vector<std::string> nodeNamesToProcess;
		std::vector<std::string> nodeCategoryStacksToProcess;

		std::unordered_map<std::string, NodeTypeData> nodeTypes;

		//this map holds pairings of the presented and the internal name
		std::unordered_map<std::string, std::string> nodeNameMap;
		std::unordered_map<ValueType, std::vector<NodePinDatabaseValue>> nodeInPinDatabase;
		std::unordered_map<ValueType, std::vector<NodePinDatabaseValue>> nodeOutPinDatabase;

		std::unordered_map<std::string, std::vector<NodePinDatabaseValue>> nodePinCodeInDatabase;
		std::unordered_map<std::string, std::vector<NodePinDatabaseValue>> nodePinCodeOutDatabase;

		std::unordered_map<std::string, NodeCategoryStack> nodeCategoryStacks;

		NodeTypeDatabase();
		~NodeTypeDatabase();

		//
		void DoProcessing();

		//temporary function to register all HLSL nodes
		void RegisterLanguage(const std::string& aLanguageFilePath);

		void CalculateNodeCategoryStack(const std::string& aNodeName, const char* aCategoryStack);
		void ProcessNodeData(const std::string& aNodeName);

		void RegisterNode(const char* aNodeName, NodeCategory aCategory, const char* aDescription, const NodeCreateFunction& aCreateFunction, const char* aCategoryStack);

		template <typename T>
		void RegisterNode(const char* aCategoryStack);
		
	};

	inline NodeTypeDatabase nodeTypeDatabase;

	template <typename T>
	void NodeTypeDatabase::RegisterNode(const char* aCategoryStack)
	{
		T node;

		const char* nodeName = typeid(T).name();
		const size_t nodeNameLength = strlen(nodeName);
		for (size_t ptr = nodeNameLength-1; ptr > 0; --ptr)
		{
			if (nodeName[ptr] == ':')
			{
				nodeName = &nodeName[ptr+1];
				break;
			}
		}

		nodeTypes[nodeName] = {
			node.GetName(),
			node.GetCategory(),
			node.GetDescription(),
			[](Script* aScript) -> ScriptNode* { ScriptNode* made = new T(); made->AssignScript(aScript); return made; }
		};

		nodeNameMap[node.GetName()] = nodeName;
		ProcessNodeData(nodeName);
		CalculateNodeCategoryStack(nodeName, aCategoryStack);
	}
}