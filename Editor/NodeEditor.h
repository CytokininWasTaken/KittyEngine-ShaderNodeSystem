#pragma once
#ifndef KITTYENGINE_NO_EDITOR
#include "Script/NodeDatabase.h"
#include "Script/Script.h"
#include "ImNodes/ImNodes.h"
#include "../Nodes/NodeEditorAction.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include "Script/CodeScriptParser.h"

#define INVALID_NODE_ID -1

struct ID3D11ShaderResourceView;

namespace KE
{
	class ScriptNode;
	struct Pin;
}

namespace KE_EDITOR
{
	constexpr char DRAG_DROP_VARIABLE_KEY[] = "DragDropVariableKey";

	class Editor;

	class NodeEditor : public EditorWindowBase
	{
		KE_EDITOR_FRIEND
	private:
		KE::Script* myScript;
		struct NodeCreationData
		{
			KE::ValueType limitType = KE::ValueType::Count;
			KE::ScriptMemberID linkFrom = {};
			bool linkFromIsOutput = false;

			bool firstFrameCreatingNode = false;
			bool creatingVariableNode = false;
			char variableName[KE::PIN_STRING_MAX] = {};

			KE::DataType* codeDataType = nullptr;
		} myNodeCreationData;

		std::vector<ImDrawList*> nodeDrawLists;

		struct CodeEditorData
		{
			KE::ParsingOutput* parsed = nullptr;
			TextEditor codeEditor;

			std::array<ID3D11ShaderResourceView*, 16> shaderTextures;
			std::unordered_map<KE::ScriptMemberID, int, KE::ScriptMemberID> nodePreviewTextureMap;
			bool regenerateShader = false;
			
		} myCodeEditorData;

		int myZoomLevel = 0;
		float myZoomScale = 1.0f;

		KE::ScriptComment commentDragPreview;

		EditorActionStack myActionStack;

	public:
		NodeEditor(EditorWindowInput aStartupData = {}) : EditorWindowBase(aStartupData) {}

		void Init() override;
		void Update() override;
		void Render() override;

		void OnEdit();

		void Serialize(void* aWorkingData) override;
		void Deserialize(void* aWorkingData) override;

		const char* GetWindowName() const override { return "Node Editor"; };

		void SetScript(KE::Script* aScript);
		KE::Script* GetScript() const { return myScript; }

		void GetSelection(std::vector<int>& selectedConnections, std::vector<int>& selectedNodes);

		int PushNodeCategoryStack(const KE::NodeTypeDatabase::NodeCategoryStack& aCategoryStack);
		void PopNodeCategoryStack(int aStackDepth);

		//Style Helpers
		static ImColor EvaluateHoverColour(const ImColor& aColour);
		static ImColor GetNodeColour(const KE::ScriptNode* aNode);
		static ImColor GetValueColour(KE::ValueType type);
		static ImColor GetPinColour(const KE::Pin* aPin);
		static const char* GetValueTypeString(KE::ValueType aValueType);

		ImVec2 NodeEditorToScreenSpace(const ImVec2& aNodeEditorPosition) const;
		ImVec2 ScreenSpaceToNodeEditor(const ImVec2& aScreenSpacePosition) const;
		void MoveComment(KE::ScriptComment& comment, ImVec2 screenSpacePos);

		ImVec4 EvaluateRegion(std::vector<KE::ScriptMemberID>& aNodes);
		void GetConnectedNodes(KE::ScriptMemberID aNodeID, std::vector<KE::ScriptMemberID>& aOutConnections);
		ImVec4 EvaluateConnectedRegion(KE::ScriptMemberID aNodeID);

		ImVec2 GetNodePosition(KE::ScriptNode* aNode) const;
		void SetNodePosition(KE::ScriptNode* aNode, const ImVec2& aPosition);

		KE::ScriptMemberID AddNode(KE::ScriptNode* aNode);
		void RemoveNode(KE::ScriptMemberID anID);
		void AddConnection(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin);
		void RemoveConnection(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin);

		void AddLinkBranch(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin);
		void MergeLinkBranch(KE::ScriptNode* node);

		void DisplayComments();

		void DeselectNode(KE::ScriptMemberID anID);

		void DisplayNodeEditor(std::unordered_map<KE::ScriptMemberID, KE::ScriptNode*, KE::ScriptMemberID>& nodes,
		                       std::unordered_map<KE::ScriptMemberID, std::vector<KE::PinConnection>, KE::ScriptMemberID>& connections,
		                       std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& tempConnections
		);
		void DisplayScriptVariable(std::unordered_map<std::string, KE::PinValue>::value_type& variable);

		void DisplayScriptAttributes();
		void HighlightCodeFromNode(TextEditor& textEditor, std::vector<int> selectedNodes, KE::ParsingOutput parsed);
		void HighlightNodeFromCode(TextEditor& textEditor, KE::ParsingOutput parsed);
		ImNodesPinShape GetPinShape(KE::Pin* aPin) const;

		KE::Pin* GetPin(KE::ScriptMemberID anID) const;
		void RenderPin(KE::ScriptNode* aNode, KE::Pin* aPin, bool aIsOutput);
		void RenderNode(KE::ScriptNode* aNode);

		int EvaluatePinWidth(KE::Pin* aPin);
		ImVec2 EvaluateNodeColumnSizes(KE::ScriptNode* aNode);
		int GetValueTypeWidth(KE::ValueType type);
		bool DisplayPinValueType(KE::PinValue* aValue, const char* label);
		bool DisplayPinValue(KE::Pin* aPin, bool aIsOutput);

		void GetConnections(KE::ScriptMemberID aNodeID, std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& aOutConnections);
		void AddConnections(const std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& aConnections);

		bool PinHasConnection(const KE::Pin* aPin) const;

		bool PinIsOutput(KE::Pin* aPin) const;

		void CreateNodePopup();
		void CreateVariableNodePopup();
		void StyleBegin() override;
		void StyleEnd() override;

		Vector4i GetTextRegionFromIndices(const std::string& aString, size_t aStartIndex, size_t aEndIndex);
	};
}
#endif