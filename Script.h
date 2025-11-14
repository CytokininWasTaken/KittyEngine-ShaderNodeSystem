#pragma once
#include "LanguageData.h"
#include "Node.h"

namespace KE
{
	class GameObjectManager;
	class CollisionHandler;
	
	struct ScriptComment
	{
		char text[KE::PIN_STRING_MAX] = {};
		Vector2f position;
		Vector2f size;
		Vector4f colour;

		enum class InteractionState
		{
			Idle,
			Moving,
			ResizingRight,
			ResizingLeft,
			ResizingTop,
			ResizingBottom,

		} interactionState = InteractionState::Idle;
	};

	struct ScriptMacro
	{
		std::vector<Pin> inputValues;
		std::vector<Pin> outputValues;

		ScriptMemberID macroStartID;
		ScriptMemberID macroEndID;

		Script* macroScript;
	};

	struct CodeRenderingContext;

	class Script
	{
	private:
		short nextNodeID = 0;

		std::unordered_map<ScriptMemberID, std::vector<PinConnection>, ScriptMemberID> connections;
		std::unordered_map<ScriptMemberID, ScriptNode*, ScriptMemberID> nodes;
		std::unordered_map<ScriptMemberID, Vector2f, ScriptMemberID> nodePositions;
		std::unordered_map<std::string, PinValue> scriptVariables;
		std::vector<ScriptComment> scriptComments;

		std::unordered_map<std::string, ScriptMacro> macros;
		std::array<std::vector<ScriptMemberID>, static_cast<int>(EntryPointType::Count)> entryPoints;

		std::string scriptName;
		std::string scriptPath;

		LanguageDefinitionNew* languageDefinition = nullptr; //this is optional, default scripts don't use a language definition
		CodeRenderingContext* codeRenderingContext = nullptr; //only used for code scripts

	public:
		std::unordered_map<ScriptMemberID, std::vector<PinConnection>, ScriptMemberID>& GetConnections() { return connections; }
		std::unordered_map<ScriptMemberID, ScriptNode*, ScriptMemberID>& GetNodes() { return nodes; }
		std::vector<ScriptComment>& GetComments() { return scriptComments; }

		ScriptMemberID IncrementNodeID();
		ScriptMemberID AddNode(ScriptNode* aNode);
		void BehaviourlessInsertNode(ScriptNode* aNode);
		void InsertNode(ScriptNode* aNode);
		void RemoveNode(ScriptMemberID anID);

		void SetScriptFileInfo(const std::string& aName, const std::string& aPath) { scriptName = aName; scriptPath = aPath; }
		const std::string& GetName() { return scriptName; }
		const std::string& GetPath() { return scriptPath; }

		bool HasNodeOfType(const std::string& anInternalName) const;

		void AddConnection(ScriptMemberID from, ScriptMemberID to, bool aTwoWay = true);
		void RemoveConnection(ScriptMemberID from, ScriptMemberID to);
		Vector2f GetNodePosition(ScriptMemberID anID) const { return nodePositions.contains(anID) ? nodePositions.at(anID) : Vector2f(); }
		void SetNodePosition(ScriptMemberID anID, const Vector2f& aPosition) { nodePositions[anID] = aPosition; }

		void SetVariable(const std::string& aName, const PinValue& aValue) { scriptVariables[aName] = aValue; }
		PinValue GetVariable(const std::string& aName) const { return scriptVariables.contains(aName) ? scriptVariables.at(aName) : PinValue(ValueType::Count); }
		std::unordered_map<std::string, PinValue>& GetVariables() { return scriptVariables; }
		std::array<std::vector<ScriptMemberID>, static_cast<int>(EntryPointType::Count)>& GetEntryPoints() { return entryPoints; }
				

		Pin* GetPin(ScriptMemberID anID) const;

		void SaveToFile(const std::string& aPath);
		void LoadFromFile(const std::string& aPath);

		void EvaluateMacros();
		void AddMacro(const char* aName) { macros[aName] = {}; macros[aName].macroScript = this; }
		bool PinIsOutput(Pin* aPin) const;
		std::unordered_map<std::string, ScriptMacro>& GetMacros() { return macros; }
		void EvaluateEntryPoints();

		void SetLanguageDefinition(LanguageDefinitionNew* aLanguage) { languageDefinition = aLanguage; };
		LanguageDefinitionNew* GetLanguageDefinition() const { return languageDefinition; }

		void SetCodeRenderingContext(CodeRenderingContext* aContext) { codeRenderingContext = aContext; }
		CodeRenderingContext* GetCodeRenderingContext() const { return codeRenderingContext; }
	};

}