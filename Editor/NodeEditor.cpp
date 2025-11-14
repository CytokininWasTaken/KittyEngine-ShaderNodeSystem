#include "stdafx.h"

#include "imnodes/imnodes_internal.h"
#include "Script/HLSLDefiner.h"
#ifndef KITTYENGINE_NO_EDITOR

#include "Window.h"
#include "NodeEditor.h"

#include "Editor/Source/Editor.h"
#include "Script/Script.h"

#include "Editor/Source/EditorUtils.h"
#include "Editor/Source/ImGui/ImGuiHandler.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include "SceneManagement/SceneManager.h"
#include "Script/CodeScriptParser.h"
#include "Script/NodeDatabase.h"
#include "Script/ScriptExecution.h"
#include "Script/ScriptManager.h"
#include "Utility/Timer.h"

void KE_EDITOR::NodeEditor::Init()
{
	myActionStack.Init(this, nullptr);
	SetWindowFlags(GetWindowFlags() | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void KE_EDITOR::NodeEditor::Update()
{
	if (ImGui::GetIO().KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Z))
		{
			if (ImGui::GetIO().KeyShift)
			{
				myActionStack.Redo();
				OnEdit();
			}
			else
			{
				myActionStack.Undo();
				OnEdit();
			}
		}

	}

	auto* graphics = KE_GLOBAL::blackboard.Get<KE::Graphics>("graphics");

	if (myScript && myScript->GetLanguageDefinition())
	{
		if (myCodeEditorData.regenerateShader)
		{
			//parse the script code
			delete myCodeEditorData.parsed; //lets not leak memory here :(
			KE::CodeScriptParser::Parse(myScript, myCodeEditorData.parsed);
			myCodeEditorData.codeEditor.SetText(myCodeEditorData.parsed->code);

			std::unordered_map<KE::ScriptMemberID, KE::PixelShader*, KE::ScriptMemberID> shaderMap;
			KE::CodeScriptParser::GeneratePreviewShaders(
				myScript,
				*myCodeEditorData.parsed,
				graphics,
				shaderMap
			);

			const std::string shaderName = std::format("ScriptPS_{}", myScript->GetName());
			graphics->GetShaderLoader().CreatePixelShaderFromScript(shaderName, myScript, "main");

			myCodeEditorData.regenerateShader = false;
		}


		myCodeEditorData.nodePreviewTextureMap.clear();
		KE::Material* material = nullptr;
		if (const auto* modelViewer = KE_GLOBAL::editor->GetEditorWindow<ModelViewer>())
		{
			material = modelViewer->GetModelData().myRenderResources[0].myMaterial;
		}
		KE::CodeScriptParser::RenderPreviewImages(
			myScript,
			*myCodeEditorData.parsed,
			myCodeEditorData.nodePreviewTextureMap,
			myScript->GetCodeRenderingContext()->shaderTextures,
			graphics,
			material
		);
		myCodeEditorData.shaderTextures = myScript->GetCodeRenderingContext()->shaderTextures;
	}
}

void KE_EDITOR::NodeEditor::GetSelection(std::vector<int>& selectedConnections, std::vector<int>& selectedNodes)
{
	if (const int selectedConnectionCount = ImNodes::NumSelectedLinks())
	{
		selectedConnections.resize(selectedConnectionCount);
		ImNodes::GetSelectedLinks(selectedConnections.data());
	}
	if (const int selectedNodeCount = ImNodes::NumSelectedNodes())
	{
		selectedNodes.resize(selectedNodeCount);
		ImNodes::GetSelectedNodes(selectedNodes.data());
	} 
}

int KE_EDITOR::NodeEditor::PushNodeCategoryStack(const KE::NodeTypeDatabase::NodeCategoryStack& aCategoryStack)
{
	int toPop = 0;
	for (int i = 0; i < aCategoryStack.names.size(); i++)
	{
		if (ImGui::BeginMenu(aCategoryStack.names[i].c_str()))
		{
			toPop++;
		}
		else
		{
			//we failed, so pop the stack and return 0 to indicate we failed
			for (int j = 0; j < toPop; j++)
			{
				ImGui::EndMenu();
			}
			return 0;
		}
	}
	return toPop;
}

void KE_EDITOR::NodeEditor::PopNodeCategoryStack(int aStackDepth)
{
	for (int i = 0; i < aStackDepth; i++)
	{
		ImGui::EndMenu();
	}
}


ImColor KE_EDITOR::NodeEditor::EvaluateHoverColour(const ImColor& aColour)
{
	//just make it a bit brighter
	ImColor hoverColour = aColour;
	hoverColour.Value.x *= 1.5f;
	hoverColour.Value.y *= 1.5f;
	hoverColour.Value.z *= 1.5f;
	return hoverColour;
}

ImColor KE_EDITOR::NodeEditor::GetNodeColour(const KE::ScriptNode* aNode)
{
	if (!aNode) { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
	switch (aNode->GetCategory())
	{
	case KE::NodeCategory::Math:		return { 0.2f, 0.6f, 0.2f, 1.0f };
	case KE::NodeCategory::Flow:		return { 0.8f, 0.2f, 0.2f, 1.0f };
	case KE::NodeCategory::Logic:		return { 0.2f, 0.6f, 0.4f, 1.0f };
	case KE::NodeCategory::Variable:	return { 0.6f, 0.2f, 0.4f, 1.0f };
	case KE::NodeCategory::Macro: 		return { 0.6f, 0.2f, 0.6f, 1.0f };
	default: return { 0.0f, 0.0f, 0.0f, 1.0f };
	}
}

ImColor KE_EDITOR::NodeEditor::GetValueColour(KE::ValueType type)
{
	switch (type)
	{
	case KE::ValueType::Bool:		return { 0.8f, 0.1f, 0.1f, 1.0f };
	case KE::ValueType::Float:		return { 0.785f, 1.00f, 0.240f, 1.0f };
	case KE::ValueType::Int:		return { 0.186f, 0.930f, 0.521f, 1.0f };
	case KE::ValueType::Vector2:	return { 0.3f, 0.7f, 0.0f, 1.0f };
	case KE::ValueType::Vector3:	return { 0.3f, 0.7f, 0.0f, 1.0f };
	case KE::ValueType::Vector4:	return { 0.3f, 0.7f, 0.0f, 1.0f };
	case KE::ValueType::Transform:	return { 0.7f, 0.7f, 0.0f, 1.0f };
	case KE::ValueType::String:		return { 0.7f, 0.0f, 0.7f, 1.0f };
	case KE::ValueType::GameObject:	return { 0.2f, 0.2f, 0.7f, 1.0f };
	case KE::ValueType::Component:	return { 0.7f, 0.7f, 0.4f, 1.0f };
	case KE::ValueType::Container:	return { 0.0f, 0.7f, 0.7f, 1.0f };
	case KE::ValueType::Any:		return { 0.7f, 0.7f, 0.7f, 1.0f };
	default:						return { 0.0f, 0.0f, 0.0f, 1.0f };
	}
}

ImColor KE_EDITOR::NodeEditor::GetPinColour(const KE::Pin* aPin)
{
	if (!aPin) { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
	if (aPin->codeData)
	{
		const auto* code = (KE::CodePin*)aPin->codeData;
		return ImColor(code->value.type.typeColour.x, code->value.type.typeColour.y, code->value.type.typeColour.z, 1.0f);
	}

	switch (aPin->type)
	{
	case KE::PinType::Flow:			return {0.8f, 0.8f, 0.8f, 1.0f};
	case KE::PinType::Value:
	case KE::PinType::OutputValue:	return GetValueColour(aPin->value.type);
	default:						return {0.0f, 0.0f, 0.0f, 1.0f};
	}
}

const char* KE_EDITOR::NodeEditor::GetValueTypeString(KE::ValueType aValueType) 
{
	switch (aValueType)
	{
	case KE::ValueType::Bool:		return "Bool";
	case KE::ValueType::Int:		return "Int";
	case KE::ValueType::Float:		return "Float";
	case KE::ValueType::Vector2:	return "Vector2";
	case KE::ValueType::Vector3:	return "Vector3";
	case KE::ValueType::Vector4:	return "Vector4";
	case KE::ValueType::String:		return "String";
	case KE::ValueType::GameObject:	return "GameObject";
	case KE::ValueType::Transform:	return "Transform";
	case KE::ValueType::Component:	return "Component";
	case KE::ValueType::Container:	return "Container";
	case KE::ValueType::CodeValue:	return "CodeValue";
	case KE::ValueType::Any:		return "Any";
	default: return "Unknown";
 	}
}

ImVec2 KE_EDITOR::NodeEditor::NodeEditorToScreenSpace(const ImVec2& aNodeEditorPosition) const
{
	const ImVec2 windowScreenSpacePos = ImGui::GetWindowPos();
	const ImVec2 nodeEditorPanning = ImNodes::EditorContextGetPanning();

	return { windowScreenSpacePos.x + aNodeEditorPosition.x + nodeEditorPanning.x, windowScreenSpacePos.y + aNodeEditorPosition.y + nodeEditorPanning.y };
}

ImVec2 KE_EDITOR::NodeEditor::ScreenSpaceToNodeEditor(const ImVec2& aScreenSpacePosition) const
{
	const ImVec2 windowScreenSpacePos = ImGui::GetWindowPos();
	const ImVec2 nodeEditorPanning = ImNodes::EditorContextGetPanning();

	return { aScreenSpacePosition.x - windowScreenSpacePos.x - nodeEditorPanning.x, aScreenSpacePosition.y - windowScreenSpacePos.y - nodeEditorPanning.y };
}

void KE_EDITOR::NodeEditor::MoveComment(KE::ScriptComment& comment, ImVec2 screenSpacePos)
{
	//are we hovering the right side edge ?
	const bool overRightEdge = ImGui::IsMouseHoveringRect(screenSpacePos + ImVec2(comment.size.x - 8.0f, 0.0f), screenSpacePos + ImVec2(comment.size.x, comment.size.y));
	const bool overLeftEdge = ImGui::IsMouseHoveringRect(screenSpacePos + ImVec2(0.0f, 0.0f), screenSpacePos + ImVec2(8.0f, comment.size.y));
	const bool overTopEdge = ImGui::IsMouseHoveringRect(screenSpacePos + ImVec2(0.0f, 0.0f), screenSpacePos + ImVec2(comment.size.x, 8.0f));
	const bool overBottomEdge = ImGui::IsMouseHoveringRect(screenSpacePos + ImVec2(0.0f, comment.size.y - 8.0f), screenSpacePos + ImVec2(comment.size.x, comment.size.y));
	const bool overRegion = ImGui::IsMouseHoveringRect(screenSpacePos, screenSpacePos + comment.size);

	if (overRightEdge && ImGui::IsMouseClicked(0)) { comment.interactionState = KE::ScriptComment::InteractionState::ResizingRight; }
	if (overLeftEdge && ImGui::IsMouseClicked(0)) { comment.interactionState = KE::ScriptComment::InteractionState::ResizingLeft; }
	if (overTopEdge && ImGui::IsMouseClicked(0)) { comment.interactionState = KE::ScriptComment::InteractionState::ResizingTop; }
	if (overBottomEdge && ImGui::IsMouseClicked(0)) { comment.interactionState = KE::ScriptComment::InteractionState::ResizingBottom; }
	if (overRegion && ImGui::IsMouseDoubleClicked(0)) { comment.interactionState = KE::ScriptComment::InteractionState::Moving; }

	if (overRightEdge || overLeftEdge) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }
	else if (overTopEdge || overBottomEdge)	{ ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); }

	const bool mouseDown = ImGui::IsMouseDown(0);
	if (mouseDown && comment.interactionState == KE::ScriptComment::InteractionState::ResizingRight)
	{
		comment.size.x += ImGui::GetIO().MouseDelta.x * (int)mouseDown;
	}
	else if ((mouseDown && comment.interactionState == KE::ScriptComment::InteractionState::ResizingLeft))
	{
		comment.size.x -= ImGui::GetIO().MouseDelta.x * (int)mouseDown;
		comment.position.x += ImGui::GetIO().MouseDelta.x * (int)mouseDown;
	}
	else if ((mouseDown && comment.interactionState == KE::ScriptComment::InteractionState::ResizingTop))
	{
		comment.size.y -= ImGui::GetIO().MouseDelta.y * (int)mouseDown;
		comment.position.y += ImGui::GetIO().MouseDelta.y * (int)mouseDown;
	}
	else if ((mouseDown && comment.interactionState == KE::ScriptComment::InteractionState::ResizingBottom))
	{
		comment.size.y += ImGui::GetIO().MouseDelta.y * (int)mouseDown;
	}
	else if (mouseDown && comment.interactionState == KE::ScriptComment::InteractionState::Moving)
	{
		comment.position = comment.position + ImGui::GetIO().MouseDelta;
	}
	else
	{
		comment.interactionState = KE::ScriptComment::InteractionState::Idle;
	}
}

ImVec4 KE_EDITOR::NodeEditor::EvaluateRegion(std::vector<KE::ScriptMemberID>& aNodes)
{
	ImVec2 min = { FLT_MAX, FLT_MAX };
	ImVec2 max = { -FLT_MAX, -FLT_MAX };

	const ImVec2 minOffset = { -25.0f, -4.0f };
	const ImVec2 maxOffset = { 25.0f, 46.0f };

	for (auto& node : aNodes)
	{
		const ImVec2 nodePos  = ImNodes::GetNodeGridSpacePos(node);
		const ImVec2 nodeSize = ImNodes::GetNodeDimensions(node);

		min.x = ImMin(min.x, nodePos.x);
		min.y = ImMin(min.y, nodePos.y);

		max.x = ImMax(max.x, nodePos.x + nodeSize.x);
		max.y = ImMax(max.y, nodePos.y + nodeSize.y);

	}
	min += minOffset;
	max += maxOffset;

	return { min.x, min.y, max.x, max.y };
}

void KE_EDITOR::NodeEditor::GetConnectedNodes(KE::ScriptMemberID aNodeID, std::vector<KE::ScriptMemberID>& aOutConnections)
{
	for (auto& connectionPair : myScript->GetConnections())
	{
		if (connectionPair.first.idParts.nodeID == aNodeID.idParts.nodeID)
		{
			for (const auto& connection : connectionPair.second)
			{
				KE::ScriptMemberID connectedID = connection.to;
				connectedID.idParts.pinID = 0;

				if (std::ranges::find(aOutConnections, connectedID) != aOutConnections.end()) { continue; } //already in the list, skip it
				aOutConnections.push_back(connectedID);
				GetConnectedNodes(connectedID, aOutConnections);
			}
		}
	}

}

ImVec4 KE_EDITOR::NodeEditor::EvaluateConnectedRegion(KE::ScriptMemberID aNodeID)
{
	std::vector<KE::ScriptMemberID> connectedNodes;
	GetConnectedNodes(aNodeID, connectedNodes);
	if (connectedNodes.empty()) { connectedNodes.push_back(aNodeID); }

	return EvaluateRegion(connectedNodes);
}

ImVec2 KE_EDITOR::NodeEditor::GetNodePosition(KE::ScriptNode* aNode) const
{
	return ImNodes::GetNodeGridSpacePos(aNode->ID);
}

void KE_EDITOR::NodeEditor::SetNodePosition(KE::ScriptNode* aNode, const ImVec2& aPosition)
{
	ImNodes::SetNodeGridSpacePos(aNode->ID, aPosition);
}

KE::ScriptMemberID KE_EDITOR::NodeEditor::AddNode(KE::ScriptNode* aNode)
{
	myActionStack.PerformAction(new AddNodeAction(aNode));
	OnEdit();
	return aNode->ID;
}

void KE_EDITOR::NodeEditor::RemoveNode(KE::ScriptMemberID anID)
{
	myActionStack.PerformAction(new RemoveNodeAction(myScript->GetNodes()[anID]));
	OnEdit();
}

void KE_EDITOR::NodeEditor::AddConnection(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin)
{
	myActionStack.PerformAction(new AddLinkAction(aFromPin, aToPin));
	OnEdit();
}

void KE_EDITOR::NodeEditor::RemoveConnection(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin)
{
	myActionStack.PerformAction(new RemoveLinkAction(aFromPin, aToPin));
	OnEdit();
}

void KE_EDITOR::NodeEditor::AddLinkBranch(KE::ScriptMemberID aFromPin, KE::ScriptMemberID aToPin)
{
	KE::Pin* tempFromPin = myScript->GetPin(aFromPin);
	const bool swap      = !PinIsOutput(tempFromPin); //we may end up with a fromPin that is an input pin, so we need to swap the pins around

	const KE::Pin* fromPin = swap ? myScript->GetPin(aToPin) : tempFromPin;
	const KE::Pin* toPin   = swap ? tempFromPin : myScript->GetPin(aToPin);
	
	KE::ScriptMemberID linkNodeID;
	KE::ScriptNode* linkNode = nullptr;

	if (fromPin->type == KE::PinType::Flow)
	{
		linkNodeID = AddNode(KE::nodeTypeDatabase.nodeTypes["FlowLinkNode"].createFunction(myScript));


		linkNode = myScript->GetNodes()[linkNodeID];
	}
	else if (fromPin->type == KE::PinType::Value ||	fromPin->type == KE::PinType::OutputValue)
	{
		linkNodeID = AddNode(KE::nodeTypeDatabase.nodeTypes["ValueLinkNode"].createFunction(myScript));


		linkNode = myScript->GetNodes()[linkNodeID];
		linkNode->GetInputPins()[0].value.type = fromPin->value.type;
		linkNode->GetInputPins()[0].codeData = fromPin->codeData;
		linkNode->GetOutputPins()[0].value.type = toPin->value.type;
		linkNode->GetOutputPins()[0].codeData = toPin->codeData;
	}

	ImNodes::SetNodeScreenSpacePos(linkNodeID, ImGui::GetMousePos() + ImVec2(-16.0f, 0.0f));

	RemoveConnection(fromPin->ID, toPin->ID);
	AddConnection(fromPin->ID, linkNode->GetInputPins()[0].ID);
	AddConnection(toPin->ID, linkNode->GetOutputPins()[0].ID);
}

void KE_EDITOR::NodeEditor::MergeLinkBranch(KE::ScriptNode* node)
{
	auto& connections = myScript->GetConnections();

	const KE::Pin* branchInputPin  = &node->GetInputPins()[0];
	const KE::Pin* branchOutputPin = &node->GetOutputPins()[0];

	KE::ScriptMemberID connectedToBranchIn = connections[branchInputPin->ID][0].to;

	std::vector<KE::ScriptMemberID> inputsToConnect;
	std::vector<KE::ScriptMemberID> outputsToConnect;

	for (auto& inputConnections : connections[branchInputPin->ID])
	{
		inputsToConnect.push_back(inputConnections.to);
	}

	for (auto& outputConnections : connections[branchOutputPin->ID])
	{
		outputsToConnect.push_back(outputConnections.to);
	}

	ImNodes::ClearNodeSelection();
	ImNodes::ClearLinkSelection();

	RemoveNode(node->ID);
	for (auto& input : inputsToConnect)
	{
		for (auto& output : outputsToConnect)
		{
			AddConnection(input, output);
			
		}
	}

}

void KE_EDITOR::NodeEditor::DisplayComments()
{
	ImDrawList* dl = ImGui::GetForegroundDrawList();

	{
		const ImVec2 screenSpacePos = NodeEditorToScreenSpace({ commentDragPreview.position });

		dl->AddRect(screenSpacePos, screenSpacePos + commentDragPreview.size, *(ImColor*)&commentDragPreview.colour);
	}

	int commentID = 0;
	for (auto& comment : myScript->GetComments())
	{
		ImGui::PushID(commentID);
		ImVec2 screenSpacePos = NodeEditorToScreenSpace({ comment.position });

		dl->AddRect(screenSpacePos, screenSpacePos + comment.size, *(ImColor*)&comment.colour,5.0f);
		ImColor backgroundCol = *(ImColor*)&comment.colour;
		backgroundCol.Value.w = 0.25f;
		dl->AddRectFilled(screenSpacePos, screenSpacePos + comment.size, backgroundCol, 5.0f);


		ImVec2 cachedCursorPos = ImGui::GetCursorPos();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, backgroundCol.Value);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  2.0f);
		ImGui::SetCursorScreenPos(screenSpacePos + ImVec2(0.0f, 0.0f));
		ImGui::ColorEdit3("##commentColour", &comment.colour.x, ImGuiColorEditFlags_NoInputs);
		ImGui::SetCursorScreenPos(screenSpacePos + ImVec2(16, 0.0f));
		const float textSize = ImMax(ImGui::CalcTextSize(comment.text).x, 128.0f);
		ImGui::SetNextItemWidth(textSize);
		ImGui::InputText("##commentText", comment.text, KE::PIN_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SetCursorScreenPos(screenSpacePos + ImVec2(textSize + 16, 0));

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		
		MoveComment(comment, screenSpacePos);
		

		ImGui::PopID();
		commentID++;
	}
}

void KE_EDITOR::NodeEditor::DeselectNode(KE::ScriptMemberID anID)
{
	ImNodes::ClearNodeSelection();
}

void ScaleDrawList(ImDrawList* aDrawList, unsigned int aStartCommand, float aScale, ImVec2 aOriginPos)
{
	for (unsigned int i = aStartCommand; i < static_cast<unsigned int>(aDrawList->VtxBuffer.size()); i++)
	{
		auto& v = aDrawList->VtxBuffer[i];
		v.pos -= aOriginPos;

		v.pos.x *= aScale;
		v.pos.y *= aScale;

		v.pos += aOriginPos;
	}

}

void ScaleClipRect(ImDrawList* aDrawList, unsigned int aStartCommand, float aScale, ImVec2 aOriginPos)
{
	for (unsigned int i = aStartCommand; i < static_cast<unsigned int>(aDrawList->CmdBuffer.size()); i++)
	{
		auto& cmd = aDrawList->CmdBuffer[i];
		cmd.ClipRect.x -= aOriginPos.x;
		cmd.ClipRect.y -= aOriginPos.y;
		cmd.ClipRect.z -= aOriginPos.x;
		cmd.ClipRect.w -= aOriginPos.y;

		cmd.ClipRect.x *= aScale;
		cmd.ClipRect.y *= aScale;
		cmd.ClipRect.z *= aScale;
		cmd.ClipRect.w *= aScale;

		cmd.ClipRect.x += aOriginPos.x;
		cmd.ClipRect.y += aOriginPos.y;
		cmd.ClipRect.z += aOriginPos.x;
		cmd.ClipRect.w += aOriginPos.y;
	}
}

void OverrideClipRect(ImDrawList* aDrawList, unsigned int aStartCommand, ImRect aRect)
{
	for (unsigned int i = aStartCommand; i < static_cast<unsigned int>(aDrawList->CmdBuffer.size()); i++)
	{
		auto& cmd = aDrawList->CmdBuffer[i];
		cmd.ClipRect.x = aRect.Min.x;
		cmd.ClipRect.y = aRect.Min.y;
		cmd.ClipRect.z = aRect.Max.x;
		cmd.ClipRect.w = aRect.Max.y;
	}
}

void ScaleDrawListNew(ImDrawList* aDrawList, unsigned int aStartCmd, float aScale, ImVec2 aOriginPos, bool aScaleClipRect)
{
	for (unsigned int i = aStartCmd; i < static_cast<unsigned int>(aDrawList->CmdBuffer.size()); i++)
	{
		auto& cmd = aDrawList->CmdBuffer[i];
		unsigned int idxCount = cmd.ElemCount;
		unsigned int idxStart = cmd.IdxOffset;

		for (unsigned int j = idxStart; j < idxStart + idxCount; j++)
		{
			auto& v = aDrawList->VtxBuffer[aDrawList->IdxBuffer[j]];
			v.pos -= aOriginPos;

			v.pos.x *= aScale;
			v.pos.y *= aScale;

			v.pos += aOriginPos;
		}

		if (!aScaleClipRect) { continue; }

		ImVec4 r = { aOriginPos.x, aOriginPos.y, aOriginPos.x, aOriginPos.y };

		auto& clMin = *(ImVec2*)&cmd.ClipRect.x;
		auto& clMax = *(ImVec2*)&cmd.ClipRect.z;

		clMax -= aOriginPos;
		clMin -= aOriginPos;
		clMin *= aScale;
		clMax *= aScale;
		clMin += aOriginPos;
		clMax += aOriginPos;
	}
}

void KE_EDITOR::NodeEditor::DisplayNodeEditor(std::unordered_map<KE::ScriptMemberID, KE::ScriptNode*, KE::ScriptMemberID>& nodes, std::unordered_map<KE::ScriptMemberID, std::vector<KE::PinConnection>, KE::ScriptMemberID>& connections, std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& tempConnections)
{
	const ImVec2 origin = ImGui::GetCursorScreenPos();

	myZoomLevel += (int)ImGui::GetIO().KeyCtrl * (ImGui::GetIO().MouseWheel < 0 ? -1 : ImGui::GetIO().MouseWheel > 0 ? 1 : 0);

	myZoomScale = powf(1.1f, (float)myZoomLevel);

	const ImVec2 availableSize = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("test", availableSize, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	auto* dl = ImGui::GetWindowDrawList();
	dl->_FringeScale = 1.0f / myZoomScale;

	const unsigned int startCommand = dl->CmdBuffer.size();

	const auto mp = ImGui::GetIO().MousePos;
	ImGui::GetIO().MousePos = (mp - origin) / myZoomScale + origin;


	nodeDrawLists.clear();
	ImNodes::BeginNodeEditor(myZoomScale);
	ImNodes::GetCurrentContext()->EditorCtx->LinkShape = ImLinkShape::Polyline;

	auto* cdl = ImNodes::GetCurrentContext()->CanvasDrawList;
	cdl->_FringeScale = 1.0f/ myZoomScale;

	ImGui::PushClipRect(origin, origin + availableSize/ myZoomScale, false);

	for (auto& nodeData : nodes)
	{
		if (!nodeData.second) { continue; }
		myScript->SetNodePosition(nodeData.first, ImNodes::GetNodeGridSpacePos(nodeData.first));
		RenderNode(nodeData.second);
	}

	tempConnections.clear();

	int connectionIndex = 0;
	std::unordered_map<KE::ScriptMemberID, std::vector<KE::ScriptMemberID>, KE::ScriptMemberID> renderedConnections;
	//ugly workaround, so we don't draw the same link twice (once for a->b and once for b->a)

	for (auto& connection : connections)
	{
		for (auto& connectionEntry : connection.second)
		{
			const bool alreadyRendered = renderedConnections[connectionEntry.to].size() > 0 && std::ranges::find(renderedConnections[connectionEntry.to], connection.first) != renderedConnections[connectionEntry.to].end();
			renderedConnections[connection.first].push_back(connectionEntry.to);

			auto baseColor = alreadyRendered ? ImColor(0, 0, 0, 0) : GetPinColour(GetPin(connection.first));
			auto hoverColor = EvaluateHoverColour(baseColor);

			ImNodes::PushColorStyle(ImNodesCol_Link, baseColor);
			ImNodes::PushColorStyle(ImNodesCol_LinkSelected, hoverColor);
			ImNodes::PushColorStyle(ImNodesCol_LinkHovered, hoverColor);

			ImNodes::Link(connectionIndex++, connection.first, connectionEntry.to);
			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();

			tempConnections.push_back({ connection.first, connectionEntry.to });
		}
	}

	ImGui::PopClipRect();
	ImNodes::EndNodeEditor();



	ScaleDrawList(cdl, 0, myZoomScale, origin);
	OverrideClipRect(cdl, 0, ImRect(origin, origin + availableSize));

	for (ImDrawList* ndl : nodeDrawLists)
	{
		ScaleDrawList(ndl, 0, myZoomScale, origin);
		OverrideClipRect(ndl, 0, ImRect(origin, origin + availableSize));
	}

	ImGui::EndChild();

	ScaleDrawList(dl, startCommand, 1.0f / myZoomScale, origin);
	OverrideClipRect(dl, startCommand, ImRect(origin, origin + availableSize * myZoomScale));

	ImGui::GetIO().MousePos = mp;

	if(ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DRAG_DROP_VARIABLE_KEY))
		{
			myNodeCreationData = {};
			myNodeCreationData.firstFrameCreatingNode = true;
			myNodeCreationData.creatingVariableNode = true;
			strcpy_s(myNodeCreationData.variableName, (char*)payload->Data);
		}
		ImGui::EndDragDropTarget();
	}
}

void KE_EDITOR::NodeEditor::DisplayScriptVariable(std::unordered_map<std::string, KE::PinValue>::value_type& variable)
{
	DisplayPinValueType(&variable.second, variable.first.c_str());

	char variableCopy[KE::PIN_STRING_MAX] = "";
	strcpy_s(variableCopy, variable.first.c_str());

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload(DRAG_DROP_VARIABLE_KEY, variableCopy, KE::PIN_STRING_MAX);
		ImGui::TextUnformatted(variable.first.c_str());
		ImGui::EndDragDropSource();
	}

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, GetValueColour(variable.second.type).Value);
	ImGui::Text("(%s)", GetValueTypeString(variable.second.type));
	ImGui::PopStyleColor();
}

void KE_EDITOR::NodeEditor::DisplayScriptAttributes()
{
	if (ImGui::Button("Save"))
	{
		myScript->SaveToFile(myScript->GetPath());
	}
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		myScript->LoadFromFile(myScript->GetPath());

		for (auto& node : myScript->GetNodes())
		{
			Vector2f pos = myScript->GetNodePosition(node.second->ID);
			ImNodes::SetNodeGridSpacePos(node.second->ID, pos);
		}
	}

#pragma region ScriptVariables
	ImGui::SeparatorText("Variables");

	for (auto& variable : myScript->GetVariables())
	{
		DisplayScriptVariable(variable);
	}

	static char variable[KE::PIN_STRING_MAX] = "";
	static char typeName[KE::PIN_STRING_MAX] = "Bool";
	static KE::ValueType type = KE::ValueType::Bool;
	ImGui::SetNextItemWidth(100);
	if (ImGui::InputTextWithHint("##newVariableField", "Name...", variable, KE::PIN_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		KE::PinValue value;
		value.type = type;
		myScript->SetVariable(variable, value);
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::PushStyleColor(ImGuiCol_Text, GetValueColour(type).Value);
	if (ImGui::BeginCombo("##newVariableType", typeName))
	{
		for (int i = (int)KE::ValueType::Bool; i < (int)KE::ValueType::Count; i++) //cursed iteration start :skull:
		{
			const char* label = GetValueTypeString((KE::ValueType)i);

			ImGui::PushStyleColor(ImGuiCol_Text, GetValueColour((KE::ValueType)i).Value);
			if (ImGui::Selectable(label))
			{
				type = (KE::ValueType)i;
				strcpy_s(typeName, label);
			}
			ImGui::PopStyleColor();

		}
		ImGui::EndCombo();
	}
	ImGui::PopStyleColor();
#pragma endregion

#pragma region ScriptMacros

	ImGui::SeparatorText("Macros");

	static char newMacroName[KE::PIN_STRING_MAX] = "";
	ImGui::SetNextItemWidth(100);
	if (ImGui::InputTextWithHint("##newMacroField", "Name...", newMacroName, KE::PIN_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		myScript->AddMacro(newMacroName);
	}

	for (auto& macro : myScript->GetMacros())
	{
		ImGui::TextUnformatted(macro.first.c_str());
		
	}

#pragma endregion
}

void KE_EDITOR::NodeEditor::HighlightCodeFromNode(TextEditor& textEditor, std::vector<int> selectedNodes, KE::ParsingOutput parsed)
{
	const KE::ScriptMemberID selectedNode = *(KE::ScriptMemberID*)&selectedNodes[0];
	const auto region                     = KE::CodeScriptParser::GetCodeAreaFromAuthor(parsed.parsingData, selectedNode, parsed.code);
	if (region.second == 0) { return; }

	Vector4i evalRegion = GetTextRegionFromIndices(parsed.code, region.first, region.second-1);
	if (evalRegion.Length() == 0) { return; }

	textEditor.SetSelection({evalRegion.y, evalRegion.x-1}, {evalRegion.w, evalRegion.z});
}

void KE_EDITOR::NodeEditor::HighlightNodeFromCode(TextEditor& textEditor, KE::ParsingOutput parsed)
{
	const std::string& currentLine = textEditor.GetCurrentLineText();
	KE::ScriptMemberID lineAuthor = KE::CodeScriptParser::GetLineAuthor(parsed.parsingData, currentLine);
				
				
	if (lineAuthor.combinedID > INT_MIN)
	{
		std::vector<KE::ScriptMemberID> author;
		author.push_back(lineAuthor);
		ImVec4 region = EvaluateRegion(author);
				
		const ImVec2 screenSpaceMin = NodeEditorToScreenSpace({ region.x, region.y });
		const ImVec2 screenSpaceMax = NodeEditorToScreenSpace({ region.z, region.w });
				
		ImGui::GetForegroundDrawList()->AddRect(screenSpaceMin, screenSpaceMax, ImColor(0.0f,1.0f,1.0f,1.0f), 5.0f);
	}
}

void KE_EDITOR::NodeEditor::Render()
{
	if (!myScript) { return; }

	auto min = ImGui::GetWindowPos();
	auto max = min + ImGui::GetWindowContentRegionMax();

	int colorPushes = 0;

	

	static KE::ScriptMemberID makingLinkID;
	if (ImNodes::IsLinkStarted((int*)&makingLinkID))
	{
		ImNodes::PushColorStyle(ImNodesCol_Link, GetPinColour(GetPin(makingLinkID)));
		ImNodes::PushColorStyle(ImNodesCol_LinkSelected, EvaluateHoverColour(GetPinColour(GetPin(makingLinkID))));
		ImNodes::PushColorStyle(ImNodesCol_LinkHovered,EvaluateHoverColour(GetPinColour(GetPin(makingLinkID))));
	}
	if (ImNodes::IsLinkDropped())
	{
		if (makingLinkID.combinedID  != INT_MIN)
		{
			KE::Pin* fromPin = GetPin(makingLinkID);

			myNodeCreationData = { fromPin->value.type, makingLinkID, PinIsOutput(fromPin) };
			if (fromPin->codeData)
			{
				auto* codePin = (KE::CodePin*)fromPin->codeData;
				myNodeCreationData.codeDataType = &codePin->value.type;
			}

			ImGui::OpenPopup("Create Node");

			makingLinkID.combinedID = INT_MIN;
		}
	}

	ImNodes::PushColorStyle(ImNodesCol_NodeOutline, ImColor(0.0f, 0.0f, 0.0f, 0.0f));
	ImNodes::PushColorStyle(ImNodesCol_GridBackground, ImColor(0.15f, 0.15f, 0.175f, 1.0f));
	colorPushes++;
	colorPushes++;
		
	auto& nodes = myScript->GetNodes();
	auto& connections = myScript->GetConnections();
	std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>> tempConnections;

	std::vector<int> selectedConnections, selectedNodes;
	GetSelection(selectedConnections, selectedNodes);

	static bool showSidebar = true;
	static bool showAttributes = true;
	static bool showCode = false;
	if (ImGui::BeginTable("##nodeEditorMainTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable))
	{
		ImGui::TableSetColumnEnabled(1, showSidebar);

		ImGui::TableNextColumn();
		DisplayNodeEditor(nodes, connections, tempConnections);

		ImGui::TableNextColumn();

		if (ImGui::BeginTabBar("##nodeEditorTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Attributes"))
			{
				showAttributes = true;
				showCode = false;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Code"))
			{
				showAttributes = false;
				showCode = true;
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		if (showAttributes)
		{
			DisplayScriptAttributes();
		}
		else if (showCode)
		{
			if (ImGui::Button("Compile VS"))
			{
				KE_GLOBAL::blackboard.Get<KE::Graphics>("graphics")->GetShaderLoader().CreateVertexShaderFromScript("scriptShader_VS", myScript);
			}
			ImGui::SameLine();
			if (ImGui::Button("Compile PS"))
			{
				auto* graphics = KE_GLOBAL::blackboard.Get<KE::Graphics>("graphics");
				const std::string shaderName = std::format("ScriptPS_{}", myScript->GetName());
				graphics->GetShaderLoader().CreatePixelShaderFromScript(shaderName, myScript, "main");
			}
			ImGui::SameLine();
			if (ImGui::Button("Rescan Language"))
			{
				KE::LanguageDefiner::DefineLanguage("Data/EngineAssets/LanguageDefinitions/hlsl.json");
			}

			myCodeEditorData.codeEditor.Render("Compiled Code");
		}


		ImGui::EndTable();
	}


	for (int i = 0; i < colorPushes; i++)
	{
		ImNodes::PopColorStyle();
	}	

	if (selectedNodes.size() > 0 && selectedNodes[0] > INVALID_NODE_ID)
	{

		if (ImGui::IsKeyReleased(ImGuiKey_H))
		{
			std::vector<KE::ScriptMemberID> connected;
			GetConnectedNodes(*(KE::ScriptMemberID*)&selectedNodes[0], connected);

			ImNodes::ClearNodeSelection();
			for (auto id : connected)
			{
				ImNodes::SelectNode(id.combinedID);
			}
		}

		ImVec4 region = EvaluateRegion(*(std::vector<KE::ScriptMemberID>*)&selectedNodes);
		
		ImVec2 screenSpaceMin = NodeEditorToScreenSpace({ region.x, region.y });
		ImVec2 screenSpaceMax = NodeEditorToScreenSpace({ region.z, region.w });

		ImGui::GetForegroundDrawList()->AddRect(screenSpaceMin, screenSpaceMax, ImColor(1.0f,1.0f,1.0f,1.0f), 5.0f);
	}


	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		int hoveredLink;
		if (ImNodes::IsLinkHovered(&hoveredLink))
		{
			AddLinkBranch(tempConnections[hoveredLink].first, tempConnections[hoveredLink].second);
		}
	}

	if (ImGui::IsKeyReleased(ImGuiKey_M))
	{
		for (int i = 0; i < selectedNodes.size(); i++)
		{
			auto* node = myScript->GetNodes()[*(KE::ScriptMemberID*)&selectedNodes[i]];
			if (strcmp(node->GetName(), "Flow Link") == 0 || strcmp(node->GetName(), "Value Link") == 0)
			{
				MergeLinkBranch(node);
			}
		}
	}

	if (ImGui::IsKeyDown(ImGuiKey_LeftAlt))
	{
		for (int i = 0; i < selectedNodes.size(); i++)
		{
			auto oldFlags = ImNodes::GetStyle().Flags;
			ImNodes::GetStyle().Flags ^= ImNodesStyleFlags_GridSnapping;
			ImNodes::SnapNodeToGrid(selectedNodes[i]);
			ImNodes::GetStyle().Flags = oldFlags;

		}
	}

	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
	{
		myActionStack.BeginMultiAction();
		for (auto& connection : selectedConnections)
		{
			RemoveConnection(tempConnections[connection].first, tempConnections[connection].second);
		}


		for (auto& nodeID : selectedNodes)
		{
			RemoveNode(*(KE::ScriptMemberID*)&nodeID);
		}
		myActionStack.EndMultiAction();
	}

	KE::ScriptMemberID fromPinID, toPinID;
	if (ImNodes::IsLinkCreated((int*)&fromPinID, (int*)&toPinID))
	{
		KE::Pin* fromPin = GetPin(fromPinID);
		KE::Pin* toPin = GetPin(toPinID);

		if (fromPin->value.type == KE::ValueType::Any && toPin->value.type == KE::ValueType::Any)
		{
			//We can't really handle updating the type if neither pin has a type, so just don't allow it
			return;
		}

		if (fromPin->value.type == KE::ValueType::Any || toPin->value.type == KE::ValueType::Any)
		{
			AddConnection(fromPinID, toPinID);
			return;
		}

		if (fromPin->type == toPin->type ||
			fromPin->type == KE::PinType::OutputValue && toPin->type == KE::PinType::Value )
		{
			if (fromPin->value.type == toPin->value.type)
			{
				AddConnection(fromPinID, toPinID);
			}
			else
			{
				short nodeID = toPinID.idParts.nodeID;
				int pinIndex = toPinID.idParts.pinID;
				KE::ScriptMemberID assignNodeID;
				assignNodeID.idParts.nodeID = nodeID;

				if (nodes[assignNodeID]->TryAssignUnsupportedPinValue(pinIndex, GetPin(fromPinID)->value))
				{
					AddConnection(fromPinID, toPinID);
				}
			}
		}
	}

	int linkID;
	if(ImNodes::IsLinkDestroyed(&linkID))
	{
		myScript->RemoveConnection(tempConnections[linkID].first, tempConnections[linkID].second);
	}

	const ImVec2 rmbDrag = ImGui::GetMouseDragDelta(1);
	const bool rmbDragged = rmbDrag.x > 32.0f && rmbDrag.y > 32.0f;
	if (rmbDragged)
	{
		commentDragPreview.position = ScreenSpaceToNodeEditor(ImGui::GetMousePos() - rmbDrag);
		commentDragPreview.size = rmbDrag;
		commentDragPreview.colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	}
	else { commentDragPreview.colour.w = 0; }

	if (ImGui::IsMouseReleased(1) && ImGui::IsMouseHoveringRect(min, max))
	{
		if (rmbDragged)
		{
			const ImVec2 nodeSpacePos = ScreenSpaceToNodeEditor(ImGui::GetMousePos() - rmbDrag);
			myScript->GetComments().push_back({"Unnamed Comment",nodeSpacePos , rmbDrag, {255,255,255,255}});
		}
		else
		{
			myNodeCreationData = {}; //reset
			ImGui::OpenPopup("Create Node");
		}
	}

	CreateNodePopup();
	CreateVariableNodePopup();
}

void KE_EDITOR::NodeEditor::OnEdit()
{
	if (myScript && myScript->GetLanguageDefinition())
	{
		myCodeEditorData.regenerateShader = true;
	}
}

void KE_EDITOR::NodeEditor::Serialize(void* aWorkingData)
{
	auto& data = *(nlohmann::json*)aWorkingData;
	data["scriptName"] = myScript->GetName();
}

void KE_EDITOR::NodeEditor::Deserialize(void* aWorkingData)
{
	auto& data = *(nlohmann::json*)aWorkingData;
	const std::string scriptName = data["scriptName"];
	SetScript(KE_GLOBAL::blackboard.Get<KE::ScriptManager>("scriptManager")->GetOrLoadScript(scriptName));
}

void KE_EDITOR::NodeEditor::SetScript(KE::Script* aScript)
{
	myScript = aScript;


	delete myCodeEditorData.parsed;
	myCodeEditorData.parsed = new KE::ParsingOutput(myScript);

	auto lang = TextEditor::LanguageDefinition::HLSL();
	const auto* hlslLangDef = (KE::HLSLDefiner::LangDefinition*)myScript->GetLanguageDefinition();


	for (auto& type : hlslLangDef->dataTypes)
	{
		TextEditor::Identifier id;
		id.mDeclaration = "";
		lang.mIdentifiers.insert(std::make_pair(type.first, id));
	}

	myCodeEditorData.codeEditor.SetLanguageDefinition(lang);
	myCodeEditorData.codeEditor.SetPalette(TextEditor::GetDarkPalette());

	OnEdit();

	//

	myActionStack.Init(this, myScript);
	if (!aScript) { KE_ERROR("Assigned Null script to Node Editor!"); return; }
	for (auto& node : myScript->GetNodes())
	{
		Vector2f pos = myScript->GetNodePosition(node.second->ID);
		ImNodes::SetNodeGridSpacePos(node.second->ID, pos);
	}
}

ImNodesPinShape KE_EDITOR::NodeEditor::GetPinShape(KE::Pin* aPin) const
{
	if (aPin->value.type == KE::ValueType::Any)
	{
		return ImNodesPinShape_QuadFilled;
	}

	switch (aPin->type)
	{
	case KE::PinType::Flow:			return PinHasConnection(aPin) ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle;
	case KE::PinType::Value:		
	case KE::PinType::OutputValue:	return PinHasConnection(aPin) ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle;
	default:						return ImNodesPinShape_Circle;
	}
}

KE::Pin* KE_EDITOR::NodeEditor::GetPin(KE::ScriptMemberID anID) const
{
	return myScript->GetPin(anID);
}

void KE_EDITOR::NodeEditor::RenderPin(KE::ScriptNode* aNode, KE::Pin* aPin, bool aIsOutput)
{
	const ImNodesPinShape shape = GetPinShape(aPin);

	ImNodes::PushColorStyle(ImNodesCol_Pin, GetPinColour(aPin));
	if (aIsOutput) {ImNodes::BeginOutputAttribute(aPin->ID, shape); }
	else		   {ImNodes::BeginInputAttribute( aPin->ID, shape); }

	switch(aPin->type)
	{
		case KE::PinType::Value:
		{
			if (DisplayPinValue(aPin,aIsOutput))
			{
				OnEdit();
			}
			break;
		}
		case KE::PinType::Flow:
		case KE::PinType::OutputValue:
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(aPin->name).x);
			ImGui::TextUnformatted(aPin->name);
			break;
		}
	}

	if (aIsOutput) { ImNodes::EndOutputAttribute(); }
	else { ImNodes::EndInputAttribute(); }
	ImNodes::PopColorStyle();
}

void KE_EDITOR::NodeEditor::RenderNode(KE::ScriptNode* aNode)
{
	if (!aNode) { return; }
	ImNodes::PushColorStyle(ImNodesCol_TitleBar, GetNodeColour(aNode));
	ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected,EvaluateHoverColour(GetNodeColour(aNode)));
	ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, EvaluateHoverColour(GetNodeColour(aNode)));

	aNode->BeginDrawNode();

	float nodeNameWidth = 0.0f;

	ImNodes::BeginNode(aNode->ID);
	if (aNode->DrawHeader())
	{
		const bool showID = false;
		std::string name;
		if (showID)
		{
			name = std::format("{} ({})", aNode->GetName(), aNode->ID.idParts.nodeID);
		}
		else
		{
			name = aNode->GetName();
		}

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(name.c_str());

		if (auto* variant = dynamic_cast<KE::IVariant*>(aNode); variant && variant->GetVariantCount() > 1)
		{
			std::string variantStr = std::format("{}/{}", variant->GetVariantIndex() + 1, variant->GetVariantCount());
			ImGui::SameLine();
			if (ImGui::Button(variantStr.c_str()))
			{
				variant->SetVariantIndex((variant->GetVariantIndex() + 1) % variant->GetVariantCount());
			}
		}

		ImNodes::EndNodeTitleBar();
		nodeNameWidth = ImGui::CalcTextSize(name.c_str()).x;
	}
	ImNodes::PopColorStyle();
	ImNodes::PopColorStyle();
	ImNodes::PopColorStyle();

	ImVec2 dims = ImGui::GetContentRegionAvail();

	aNode->CustomRender();

	auto& inputPins = aNode->GetInputPins();
	auto& outputPins = aNode->GetOutputPins();

	ImVec2 columnSizes = EvaluateNodeColumnSizes(aNode);
	columnSizes.x = ImMax(columnSizes.x, 1.0f);
	columnSizes.y = ImMax(columnSizes.y, 1.0f);

	const float unusedSpace = nodeNameWidth - (columnSizes.x + columnSizes.y);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);

	ImGui::BeginChild("##inputPins", ImVec2(columnSizes.x, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	{
		for (int i = 0; i < inputPins.size(); i++)
		{
			RenderPin(aNode, &inputPins[i], false);
		}
	}
	nodeDrawLists.push_back(ImGui::GetWindowDrawList());
	ImGui::EndChild();
	ImGui::SameLine(0, unusedSpace);

	ImGui::BeginChild("##outputPins", ImVec2(columnSizes.y, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	{
		for (int i = 0; i < outputPins.size(); i++)
		{
			RenderPin(aNode, &outputPins[i], true);
		}
	}
	nodeDrawLists.push_back(ImGui::GetWindowDrawList());
	ImGui::EndChild();
	ImGui::PopStyleColor();


	if (myCodeEditorData.nodePreviewTextureMap.contains(aNode->ID))
	{
		const auto* rt = KE_GLOBAL::blackboard.Get<KE::Graphics>("graphics")->GetRenderTarget(9);
		const int index = myCodeEditorData.nodePreviewTextureMap[aNode->ID];

		const ImVec2 previewRenderedSize = { 256, 256 };
		const ImVec2 previewSize = { 128, 128 };
		const float viewportX = (float)(index % 16) * previewRenderedSize.x;
		const float viewportY = (float)(index / 16) * previewRenderedSize.y;

		const ImVec2 uv0 = {viewportX / 4096.0f, viewportY / 4096.0f};
		const ImVec2 uv1 = {uv0.x + 1.0f / 16.0f, uv0.y + 1.0f / 16.0f};

		ImGui::Image(rt->GetShaderResourceView(), previewSize, uv0, uv1);
	}


	ImNodes::EndNode();

	aNode->EndDrawNode();
}

int KE_EDITOR::NodeEditor::EvaluatePinWidth(KE::Pin* aPin)
{
	const int pinNameLength = static_cast<int>(ImGui::CalcTextSize(aPin->name).x);
	constexpr int padding = 0;

	switch (aPin->type)
	{
		case KE::PinType::Flow:			return pinNameLength + padding;
		case KE::PinType::Value:		
		{
			if (PinHasConnection(aPin) && !PinIsOutput(aPin))
			{
				return pinNameLength + padding;
			}
			return pinNameLength + GetValueTypeWidth(aPin->value.type) + padding;
		}
		case KE::PinType::OutputValue:	return pinNameLength + padding;
		default:						return 0;
	}
}

ImVec2 KE_EDITOR::NodeEditor::EvaluateNodeColumnSizes(KE::ScriptNode* aNode)
{
	int largestInputPin = 0;
	int largestOutputPin = 0;
	for (auto& pin : aNode->GetInputPins())
	{
		if (int width = EvaluatePinWidth(&pin); width > largestInputPin)
		{
			largestInputPin = width;
		}
	}

	for (auto& pin : aNode->GetOutputPins())
	{
		if (int width = EvaluatePinWidth(&pin); width > largestOutputPin)
		{
			largestOutputPin = width;
		}
	}

	return { (float)largestInputPin, (float)largestOutputPin };
}

int KE_EDITOR::NodeEditor::GetValueTypeWidth(KE::ValueType type)
{
	switch(type)
	{
		case KE::ValueType::Bool:		return 20;
		case KE::ValueType::Float:		return 50;
		case KE::ValueType::Int:		return 50;
		case KE::ValueType::Vector2:	return 200;
		case KE::ValueType::Vector3:	return 200;
		case KE::ValueType::Vector4:	return 200;
		case KE::ValueType::String:		return 100;
		case KE::ValueType::CodeValue:	return 100;
		default:						return 0;
	}
}

bool KE_EDITOR::NodeEditor::DisplayPinValueType(KE::PinValue* aValue, const char* label)
{

	switch(aValue->type)
	{
	case KE::ValueType::Bool:
	{
		return ImGui::Checkbox(label, &aValue->value.Bool);
	}
	case KE::ValueType::Float:
	{
		return ImGui::DragFloat(label, &aValue->value.Float, 0.1f);
	}
	case KE::ValueType::Int:
	{
		return ImGui::DragInt(label, &aValue->value.Int, 0.1f);
	}
	case KE::ValueType::Vector2:
	{
		return ImGui::DragFloat2(label, &aValue->value.Vector2.x, 0.1f);
	}
	case KE::ValueType::Vector3:
	{
		return ImGui::DragFloat3(label, &aValue->value.Vector3.x, 0.1f);
	}
	case KE::ValueType::Vector4:
	{
		return ImGui::DragFloat4(label, &aValue->value.Vector4.x, 0.1f);
	}
	case KE::ValueType::String:
	{
		return ImGui::InputTextWithHint(label, "...", aValue->value.String, KE::PIN_STRING_MAX);
	}
	case KE::ValueType::Transform:
	{
		return false;
	}
	default:
	{
		return false;
	}
	}
}

bool KE_EDITOR::NodeEditor::DisplayPinValue(KE::Pin* aPin, bool aIsOutput)
{
	const char* label = aIsOutput ? FormatString("%s##%d", aPin->name, aPin->ID.idParts.pinID) : FormatString("##%d", aPin->ID.idParts.pinID);
	if (!aIsOutput)
	{
		ImGui::TextUnformatted(aPin->name);
		if (PinHasConnection(aPin)) { return false; }
		ImGui::SameLine();
	}
	else
	{
		//right align
		const int width = EvaluatePinWidth(aPin);
		KE::ScriptMemberID nodeID = aPin->ID;
		nodeID.idParts.pinID = 0;

		ImGui::SetCursorPosX(ImNodes::GetNodeEditorSpacePos(nodeID).x + ImNodes::GetNodeDimensions(nodeID).x - width);
	}

	ImGui::SetNextItemWidth(static_cast<float>(GetValueTypeWidth(aPin->value.type)));

	if (aPin->codeData)
	{
		const auto* codePin = (KE::CodePin*)aPin->codeData;
		return KE::LanguageInspector::InspectType(codePin->value.type, &aPin->value.value, label);
	}

	return DisplayPinValueType(&aPin->value, label);
}

void KE_EDITOR::NodeEditor::GetConnections(KE::ScriptMemberID aNodeID, std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& aOutConnections)
{
	for (auto& pin : myScript->GetNodes().at(aNodeID)->GetInputPins())
	{
		if (PinHasConnection(&pin))
		{
			for (auto& connection : myScript->GetConnections().at(pin.ID))
			{
				aOutConnections.push_back({ pin.ID, connection.to });
			}
		}
	}
	for (auto& pin : myScript->GetNodes().at(aNodeID)->GetOutputPins())
	{
		if (PinHasConnection(&pin))
		{
			for (auto& connection : myScript->GetConnections().at(pin.ID))
			{
				aOutConnections.push_back({ pin.ID, connection.to });
			}
		}
	}
}

void KE_EDITOR::NodeEditor::AddConnections(const std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>>& aConnections)
{
	for (auto& connection : aConnections)
	{
		AddConnection(connection.first, connection.second);
	}
}

bool KE_EDITOR::NodeEditor::PinHasConnection(const KE::Pin* aPin) const
{
	const auto& find = myScript->GetConnections().find(aPin->ID);
	if (find != myScript->GetConnections().end() && find->second.size() > 0) { return true; }
	return false;
}

bool KE_EDITOR::NodeEditor::PinIsOutput(KE::Pin* aPin) const
{
	return myScript->PinIsOutput(aPin);
}

void KE_EDITOR::NodeEditor::CreateNodePopup()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

	static char filter[KE::PIN_STRING_MAX] = "";

	if (ImGui::BeginPopup("Create Node"))
	{
		const ImVec2 createPos = ImGui::GetCursorScreenPos();
		if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); ClearString(filter, KE::PIN_STRING_MAX); }
		if (ImGui::InputTextWithHint("##filter", "Filter...", filter, KE::PIN_STRING_MAX)) {}

		KE::ScriptMemberID makeConnectionID; makeConnectionID.combinedID = INT_MIN;

		if (myNodeCreationData.codeDataType)
		{
			const auto& matchList = (myNodeCreationData.linkFromIsOutput) ?
				KE::nodeTypeDatabase.nodePinCodeInDatabase[myNodeCreationData.codeDataType->typeName] :
				KE::nodeTypeDatabase.nodePinCodeOutDatabase[myNodeCreationData.codeDataType->typeName];

			for (auto& match : matchList)
			{
				const char* nicerName = KE::nodeTypeDatabase.nodeTypes[match.nodeName].name.c_str();
				if (!MatchesFilter(filter, nicerName)) { continue; }
				if (const int push = PushNodeCategoryStack(KE::nodeTypeDatabase.nodeCategoryStacks[match.nodeName]))
				{
					if (ImGui::MenuItem(nicerName))
					{
						KE::ScriptNode* node = KE::nodeTypeDatabase.nodeTypes[match.nodeName].createFunction(myScript);
						AddNode(node);

						if (match.variantIndex > 0)
						{
							if (auto* variant = dynamic_cast<KE::IVariant*>(node))
							{
								variant->SetVariantIndex(match.variantIndex);
							}
						}

						ImNodes::SetNodeScreenSpacePos(node->ID, createPos);
						const int pinIndex = match.index;
						makeConnectionID = myNodeCreationData.linkFromIsOutput ? node->GetInputPins()[pinIndex].ID : node->GetOutputPins()[pinIndex].ID;
					}
					PopNodeCategoryStack(push);
				}
			}

		}
		else if (myNodeCreationData.limitType == KE::ValueType::Count)
		{
			for (auto& type : KE::nodeTypeDatabase.nodeTypes)
			{
				if (!MatchesFilter(filter, type.second.name.c_str())) { continue; }
				if (const int push = PushNodeCategoryStack(KE::nodeTypeDatabase.nodeCategoryStacks[type.first]))
				{
					if (KE::nodeTypeDatabase.nodeCategoryStacks[type.first].names.back() == "Macro")
					{
						for (auto& macro : myScript->GetMacros())
						{
							if (ImGui::BeginMenu(macro.first.c_str()))
							{
								if (ImGui::MenuItem(type.second.name.c_str()))
								{
									myScript->EvaluateMacros();
									KE::ScriptMemberID nodeID = AddNode(type.second.createFunction(myScript));
									ImNodes::SetNodeScreenSpacePos(nodeID, createPos);

									struct
									{
										char macroName[KE::PIN_STRING_MAX];
										KE::ScriptMacro* nodeMacro;
									} outData;
									strcpy_s(outData.macroName, macro.first.c_str());
									outData.nodeMacro = &macro.second;

									myScript->GetNodes()[nodeID]->SetCustomData(&outData);

								}
								ImGui::EndMenu();
							}
						}
					}
					else
					{
						if (ImGui::MenuItem(type.second.name.c_str()))
						{
							KE::ScriptMemberID nodeID = AddNode(type.second.createFunction(myScript));
							ImNodes::SetNodeScreenSpacePos(nodeID, createPos);
						}
					}
					PopNodeCategoryStack(push);
				}
			}
		}
		else
		{
			const auto& matchList = (myNodeCreationData.linkFromIsOutput) ?
				KE::nodeTypeDatabase.nodeInPinDatabase[myNodeCreationData.limitType] :
				KE::nodeTypeDatabase.nodeOutPinDatabase[myNodeCreationData.limitType];

			for (auto& match : matchList)
			{
				const char* nicerName = KE::nodeTypeDatabase.nodeTypes[match.nodeName].name.c_str();
				if (!MatchesFilter(filter, nicerName)) { continue; }
				if (const int push = PushNodeCategoryStack(KE::nodeTypeDatabase.nodeCategoryStacks[match.nodeName]))
				{
					if (ImGui::MenuItem(nicerName))
					{
						KE::ScriptNode* node = KE::nodeTypeDatabase.nodeTypes[match.nodeName].createFunction(myScript);
						AddNode(node);

						ImNodes::SetNodeScreenSpacePos(node->ID, createPos);
						const int pinIndex = match.index;
						makeConnectionID = myNodeCreationData.linkFromIsOutput ? node->GetInputPins()[pinIndex].ID : node->GetOutputPins()[pinIndex].ID;
					}
					PopNodeCategoryStack(push);
				}
			}
		}

		if (makeConnectionID.combinedID != INT_MIN)
		{
			myScript->AddConnection(myNodeCreationData.linkFrom, makeConnectionID);
		}

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

void KE_EDITOR::NodeEditor::CreateVariableNodePopup()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (myNodeCreationData.firstFrameCreatingNode)
	{
		ImGui::OpenPopup("Create Variable Node");
		myNodeCreationData.firstFrameCreatingNode = false;
	}

	if (ImGui::BeginPopup("Create Variable Node"))
	{
		ImGui::Text(myNodeCreationData.variableName);
		ImGui::Separator();
		if (ImGui::MenuItem("Set"))
		{
			KE::ScriptNode* node = KE::nodeTypeDatabase.nodeTypes["SetVariableNode"].createFunction(myScript);
			myScript->AddNode(node);
			ImNodes::SetNodeScreenSpacePos(node->ID, ImGui::GetMousePos());
			node->GetInputPins()[1].value.type = myScript->GetVariable(myNodeCreationData.variableName).type;
			strcpy_s(node->GetInputPins()[1].name, myNodeCreationData.variableName);

		}
		if (ImGui::MenuItem("Get"))
		{
			KE::ScriptNode* node = KE::nodeTypeDatabase.nodeTypes["GetVariableNode"].createFunction(myScript);
			myScript->AddNode(node);
			ImNodes::SetNodeScreenSpacePos(node->ID, ImGui::GetMousePos());
			node->GetOutputPins()[0].value.type = myScript->GetVariable(myNodeCreationData.variableName).type;

			strcpy_s(node->GetOutputPins()[0].name, myNodeCreationData.variableName);
		}

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

void KE_EDITOR::NodeEditor::StyleBegin()
{
	//remove window padding
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void KE_EDITOR::NodeEditor::StyleEnd()
{
	ImGui::PopStyleVar();
}

inline Vector4i KE_EDITOR::NodeEditor::GetTextRegionFromIndices(const std::string& aString, size_t aStartIndex, size_t aEndIndex)
{
	Vector4i region = { 0, 0, 0, 0 };

	const size_t beginRow = std::count(aString.begin(), aString.begin() + aStartIndex, '\n');
	const size_t endRow = std::count(aString.begin(), aString.begin() + aEndIndex, '\n');

	const size_t beginColumn = aStartIndex - (aStartIndex > 0 ? aString.rfind('\n', aStartIndex - 1) : 0);
	const size_t endColumn = aEndIndex - (aEndIndex > 0 ? aString.rfind('\n', aEndIndex - 1) : 0);

	region.x = static_cast<int>(beginColumn);
	region.y = static_cast<int>(beginRow);

	region.z = static_cast<int>(endColumn);
	region.w = static_cast<int>(endRow);

	return region;

}

#endif